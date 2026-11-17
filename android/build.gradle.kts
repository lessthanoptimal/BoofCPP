// Builds the native library for Android ABIs and packages it as an AAR.
// Requires an Android SDK with the NDK installed.
plugins {
    alias(libs.plugins.android.library)
    `maven-publish`
    signing
}

repositories {
    google()
    mavenCentral()
}

android {
    namespace = "org.boofcpp"
    compileSdk = 35
    // Default NDK of AGP 8.9
    ndkVersion = "27.0.12077973"

    defaultConfig {
        minSdk = 22

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++17", "-fexceptions", "-frtti")
                // 16 KB page size support, required for Android 15+ devices (default starting with NDK r28)
                arguments += "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            // AGP 8 defaults to RelWithDebInfo (-O2). Use Release so the -O3 in CMakeLists.txt is applied
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                }
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    testOptions {
        targetSdk = 33
    }

    lint {
        abortOnError = false
        targetSdk = 33
    }

    publishing {
        singleVariant("release") {
            withSourcesJar()
        }
    }
}

publishing {
    publications {
        register<MavenPublication>("release") {
            groupId = project.group.toString()
            artifactId = project.name
            version = project.version.toString()
            afterEvaluate { from(components["release"]) }
            pom { boofcppPom("BoofCPP Android", "Native libraries of BoofCPP for Android") }
        }
    }
    repositories { mavenCentralPublishing(project) }
}

if (hasPublishingCredentials()) {
    signing { sign(publishing.publications["release"]) }
}
