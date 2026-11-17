// Fast: results within floating point tolerance of BoofCV
plugins {
    id("boofcpp.native-conventions")
}

nativeLibrary {
    name = "JNIBoofCPPFast"
    directory = "boofcpp-fast-jni"
}

dependencies {
    api(project(":boofcpp-common"))
    api(libs.boofcv.ip)

    testImplementation(testFixtures(project(":boofcpp-common")))
    // The benchmarks compare against strict
    "benchmarkImplementation"(project(":boofcpp-jni"))
}

tasks.named<JavaExec>("accuracyRun") { mainClass = "org.boofcpp.fast.Accuracy" }
