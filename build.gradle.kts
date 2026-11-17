// Modules are configured by the convention plugins in buildSrc, and dependency versions are in
// gradle/libs.versions.toml. The group and version are in gradle.properties
tasks.wrapper {
    distributionType = Wrapper.DistributionType.BIN
    gradleVersion = "9.2.0"
}
