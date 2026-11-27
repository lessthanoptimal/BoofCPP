pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

// Downloads the JDK of the Java toolchain if it isn't installed
plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "1.0.0"
}

rootProject.name = "BoofCPP"

include("boofcpp-common")
project(":boofcpp-common").projectDir = file("libs/common/jni")
include("boofcpp-jni")
project(":boofcpp-jni").projectDir = file("libs/strict/jni")
include("boofcpp-fast")
project(":boofcpp-fast").projectDir = file("libs/fast/jni")
include("boofcpp-benchmark")
project(":boofcpp-benchmark").projectDir = file("libs/benchmark")

// The Android module is optional so that the rest of the project builds without an Android SDK.
// The SDK is located the same way the Android Gradle Plugin does it.
fun hasAndroidSdk(): Boolean {
    if (System.getenv("ANDROID_HOME") != null || System.getenv("ANDROID_SDK_ROOT") != null)
        return true
    val local = file("local.properties")
    if (!local.exists())
        return false
    val props = java.util.Properties()
    local.inputStream().use { props.load(it) }
    return props.getProperty("sdk.dir") != null
}

if (hasAndroidSdk()) {
    include("boofcpp-android")
    project(":boofcpp-android").projectDir = file("android")
} else {
    logger.warn("Skipping boofcpp-android because no Android SDK was found (ANDROID_HOME, ANDROID_SDK_ROOT or local.properties sdk.dir)")
}
