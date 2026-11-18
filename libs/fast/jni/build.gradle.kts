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
    // Fast's native code uses strict's for what it doesn't speed up, and BoofCPP.initialize() registers fast
    api(project(":boofcpp-jni"))
    api(libs.boofcv.ip)

    testImplementation(testFixtures(project(":boofcpp-common")))
}

tasks.named<JavaExec>("accuracyRun") { mainClass = "org.boofcpp.fast.Accuracy" }
