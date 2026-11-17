// Strict: results identical to BoofCV
plugins {
    id("boofcpp.native-conventions")
}

nativeLibrary {
    name = "JNIBoofCPP"
    directory = "boofcpp-jni"
}

dependencies {
    api(project(":boofcpp-common"))
    api(libs.boofcv.ip)
    api(libs.boofcv.ip.multiview)

    testImplementation(testFixtures(project(":boofcpp-common")))
}
