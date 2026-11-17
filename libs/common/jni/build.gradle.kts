// Java code every variant shares. The test fixtures have the helpers shared by the variants' tests
plugins {
    id("boofcpp.java-conventions")
    `java-test-fixtures`
    id("com.peterabeles.gversion")
}

// org.boofcpp.BoofCPPVersion with the version and git information. It's generated before compiling and isn't committed
gversion {
    srcDir = "src/main/java"
    classPackage = "org.boofcpp"
    className = "BoofCPPVersion"
}

tasks.named("compileJava") { dependsOn("createVersionFile") }
tasks.named("sourcesJar") { dependsOn("createVersionFile") }

dependencies {
    // NativeImageBorder reads BoofCV's borders
    api(libs.boofcv.ip)
    testFixturesApi(platform(libs.junit.bom))
    testFixturesApi(libs.junit.jupiter.api)
    testFixturesApi(libs.boofcv.ip)
    testFixturesApi(libs.boofcv.io)
    // NativeConfigs compares the disparity configurations
    testImplementation(libs.boofcv.ip.multiview)
}
