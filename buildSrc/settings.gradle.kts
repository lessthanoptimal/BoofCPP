// The main build's version catalog, so plugin versions are in gradle/libs.versions.toml
dependencyResolutionManagement {
    versionCatalogs {
        create("libs") {
            from(files("../gradle/libs.versions.toml"))
        }
    }
}
