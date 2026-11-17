plugins {
    `java-library`
    `maven-publish`
    signing
}

val libs = the<VersionCatalogsExtension>().named("libs")

java {
    withJavadocJar()
    withSourcesJar()
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

// Fail on jar conflict
configurations.configureEach { resolutionStrategy { failOnVersionConflict() } }

dependencies {
    testImplementation(platform(libs.findLibrary("junit-bom").get()))
    testImplementation(libs.findLibrary("junit-jupiter-api").get())
    testRuntimeOnly(libs.findLibrary("junit-jupiter-engine").get())
    testRuntimeOnly(libs.findLibrary("junit-platform-launcher").get())
}

tasks.withType<JavaCompile>().configureEach {
    options.release = 11
}

tasks.test {
    useJUnitPlatform()
    // JDK 24+ warns when native libraries are loaded unless native access is enabled
    jvmArgs("--enable-native-access=ALL-UNNAMED")
}

tasks.javadoc {
    isFailOnError = false
}

publishing {
    publications {
        create<MavenPublication>("mavenJava") {
            from(components["java"])
            pom { boofcppPom("BoofCPP", "A C++ port of BoofCV") }
        }
    }
    repositories { mavenCentralPublishing(project) }
}

if (hasPublishingCredentials()) {
    signing { sign(publishing.publications["mavenJava"]) }
}
