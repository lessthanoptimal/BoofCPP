// JMH benchmarks of every variant and BoofCV's Java code, through BoofCV's API. Not published
plugins {
    java
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(25)
        vendor = JvmVendorSpec.AZUL
    }
}

repositories {
    mavenLocal()
    mavenCentral()
    maven("https://central.sonatype.com/repository/maven-snapshots/")
}

dependencies {
    // Fast also brings strict
    implementation(project(":boofcpp-fast"))
    implementation(testFixtures(project(":boofcpp-common")))
    implementation(libs.jmh.core)
    annotationProcessor(libs.jmh.generator.annprocess)
}

tasks.withType<JavaCompile>().configureEach {
    options.release = 11
}

// Runs the benchmarks with BenchmarkRunner, which picks the implementations of each op, and writes each
// implementation's results to build/reports/benchmark. -Pinclude=<regex> runs only the matching benchmarks,
// -Pimpl=<names> only those implementations, separated by commas, -Pthreads=<counts> those thread counts instead of 1
// and every core, -Pquick one short measurement to check that they run, and -Psave also saves each implementation
// that ran in results/benchmark/<platform>. Only a full run of every op can be saved
tasks.register<JavaExec>("benchmarkRun") {
    description = "Runs the JMH benchmarks of every implementation"
    classpath = sourceSets.main.get().runtimeClasspath
    mainClass = "org.boofcpp.benchmark.BenchmarkRunner"
    jvmArgs("--enable-native-access=ALL-UNNAMED")
    // NativeLoader finds the native build directory from the repository root
    workingDir = rootDir

    val runnerArgs = mutableListOf("--out", layout.buildDirectory.dir("reports/benchmark").get().asFile.path)
    providers.gradleProperty("include").orNull?.let { runnerArgs += listOf("--include", it) }
    providers.gradleProperty("impl").orNull?.let { runnerArgs += listOf("--impl", it) }
    providers.gradleProperty("threads").orNull?.let { runnerArgs += listOf("--threads", it) }
    if (providers.gradleProperty("quick").isPresent)
        runnerArgs += listOf("--quick", "true")
    if (providers.gradleProperty("save").isPresent)
        runnerArgs += listOf("--save", rootProject.file("results/benchmark/${platformName()}").path)
    args(runnerArgs)
}
