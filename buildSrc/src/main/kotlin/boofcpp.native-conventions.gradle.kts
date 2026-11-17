// A Java module with a JNI library built by CMake: the native jar for this platform, JMH benchmarks and the accuracy
// tool
plugins {
    id("boofcpp.java-conventions")
}

val libs = the<VersionCatalogsExtension>().named("libs")

val nativeLibrary = extensions.create<NativeLibraryExtension>("nativeLibrary")

// The CMake build directory, relative to the repository root. Change it with -Pnative_build_location=<directory>
val nativeBuild: File = rootProject.file(providers.gradleProperty("native_build_location").getOrElse("build"))
if (!nativeBuild.exists())
    throw GradleException("Native build directory $nativeBuild does not exist!")

// Classifier of the native jar, e.g. linux-amd64
val osArch = System.getProperty("os.arch").lowercase()
val osName = System.getProperty("os.name").lowercase()
val platform = when {
    osName.contains("linux") -> "linux-$osArch"
    osName.contains("mac") -> "macos-$osArch"
    osName.contains("windows") -> "windows-$osArch"
    File.separatorChar == '/' -> "unix-$osArch"
    else -> "unknown_$osArch"
}

println("  Platform:        ${System.getProperty("os.name")} ${System.getProperty("os.version")} $osArch")
println("  Classifier name: $platform")
println("  Native location: $nativeBuild")

val nativeJar by tasks.registering(Jar::class) {
    archiveClassifier = platform
    from(nativeLibrary.directory.map { nativeBuild.resolve(it) })
    // Evaluated when the jar is built, after the module has set nativeLibrary
    include { it.name.startsWith("lib${nativeLibrary.name.get()}.") }
}

publishing.publications.named<MavenPublication>("mavenJava") { artifact(nativeJar) }

// Loads the JNI library from the native jar, the way users get it. It runs in an empty directory, so the native build
// directory isn't found. CI also renames the native build directory, so a library that links another one from it
// fails
tasks.register<JavaExec>("nativeJarCheck") {
    description = "Loads the JNI library from the native jar alone"
    classpath = files(nativeJar) + sourceSets.main.get().runtimeClasspath
    mainClass = "org.boofcpp.NativeLoader"
    workingDir = temporaryDir
    jvmArgs("--enable-native-access=ALL-UNNAMED")
    argumentProviders.add(CommandLineArgumentProvider { listOf(nativeLibrary.name.get(), nativeLibrary.directory.get()) })
}

val benchmark by sourceSets.creating {
    compileClasspath += sourceSets.main.get().output + sourceSets.main.get().compileClasspath
    runtimeClasspath += sourceSets.main.get().output + sourceSets.main.get().runtimeClasspath
}

// Measures how far the native results are from BoofCV's Java code. See CodeStandards.md, Accuracy
val accuracy by sourceSets.creating {
    compileClasspath += sourceSets.main.get().output + sourceSets.main.get().compileClasspath
    runtimeClasspath += sourceSets.main.get().output + sourceSets.main.get().runtimeClasspath
}

dependencies {
    "accuracyImplementation"(testFixtures(project(":boofcpp-common")))
    "benchmarkImplementation"(libs.findLibrary("jmh-core").get())
    "benchmarkImplementation"(libs.findLibrary("jmh-generator-annprocess").get())
    "benchmarkAnnotationProcessor"(libs.findLibrary("jmh-generator-annprocess").get())
    "benchmarkImplementation"(testFixtures(project(":boofcpp-common")))
}

// Runs the JMH benchmarks matching -Pinclude=<regex>. Results are saved in build/reports/benchmark
tasks.register<JavaExec>("benchmarkRun") {
    description = "Executing JMH benchmarks"
    classpath = benchmark.runtimeClasspath + sourceSets.main.get().runtimeClasspath
    mainClass = "org.openjdk.jmh.Main"
    jvmArgs("--enable-native-access=ALL-UNNAMED")

    val include = providers.gradleProperty("include").getOrElse(".*Benchmark*")
    val format = providers.gradleProperty("format").getOrElse("json")
    val resultFile = layout.buildDirectory.file("reports/benchmark/result.$format").get().asFile
    doFirst { resultFile.parentFile.mkdirs() }

    args(include, "-rf", format, "-rff", resultFile)
}

// Runs the module's accuracy tool, the main class set by the module, and saves its record in results/accuracy
tasks.register<JavaExec>("accuracyRun") {
    description = "Measures the accuracy of the native code against BoofCV's Java code"
    classpath = accuracy.runtimeClasspath
    jvmArgs("--enable-native-access=ALL-UNNAMED")
}
