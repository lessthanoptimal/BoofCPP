// A Java module with a JNI library built by CMake: the native jar for this platform and the accuracy tool
plugins {
    id("boofcpp.java-conventions")
}

val nativeLibrary = extensions.create<NativeLibraryExtension>("nativeLibrary")

// The CMake build directory, relative to the repository root. Change it with -Pnative_build_location=<directory>
val nativeBuild: File = rootProject.file(providers.gradleProperty("native_build_location").getOrElse("build"))
if (!nativeBuild.exists())
    throw GradleException("Native build directory $nativeBuild does not exist!")

// Classifier of the native jar, e.g. linux-amd64
val platform = platformName()

println("  Platform:        ${System.getProperty("os.name")} ${System.getProperty("os.version")} ${System.getProperty("os.arch")}")
println("  Classifier name: $platform")
println("  Native location: $nativeBuild")

// The JNI library and the common library it links, which every variant's native jar has
val nativeJar by tasks.registering(Jar::class) {
    archiveClassifier = platform
    from(nativeLibrary.directory.map { nativeBuild.resolve(it) })
    from(nativeBuild.resolve("boofcpp-common"))
    // Evaluated when the jar is built, after the module has set nativeLibrary
    include { it.name.startsWith("lib${nativeLibrary.name.get()}.") || it.name.startsWith("libBoofCPPCommon.") }
}

publishing.publications.named<MavenPublication>("mavenJava") { artifact(nativeJar) }

// Loads the JNI library from the native jar, the way users get it. It runs in an empty directory, so the native build
// directory isn't found. CI also renames the native build directory, so the JNI library must find the common library
// that NativeLoader loaded from the jar, not the one in the native build directory
tasks.register<JavaExec>("nativeJarCheck") {
    description = "Loads the JNI library from the native jar alone"
    classpath = files(nativeJar) + sourceSets.main.get().runtimeClasspath
    mainClass = "org.boofcpp.NativeLoader"
    workingDir = temporaryDir
    jvmArgs("--enable-native-access=ALL-UNNAMED")
    argumentProviders.add(CommandLineArgumentProvider { listOf(nativeLibrary.name.get(), nativeLibrary.directory.get()) })
}

// Measures how far the native results are from BoofCV's Java code. See CodeStandards.md, Accuracy
val accuracy by sourceSets.creating {
    compileClasspath += sourceSets.main.get().output + sourceSets.main.get().compileClasspath
    runtimeClasspath += sourceSets.main.get().output + sourceSets.main.get().runtimeClasspath
}

dependencies {
    "accuracyImplementation"(testFixtures(project(":boofcpp-common")))
}

// Runs the module's accuracy tool, the main class set by the module, and saves its record in results/accuracy
tasks.register<JavaExec>("accuracyRun") {
    description = "Measures the accuracy of the native code against BoofCV's Java code"
    classpath = accuracy.runtimeClasspath
    jvmArgs("--enable-native-access=ALL-UNNAMED")
}
