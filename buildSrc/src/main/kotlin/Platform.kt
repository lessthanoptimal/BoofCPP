/// Name of this platform, e.g. linux-amd64. It's the native jar's classifier and the directory of saved benchmarks
fun platformName(): String {
    val osArch = System.getProperty("os.arch").lowercase()
    val osName = System.getProperty("os.name").lowercase()
    return when {
        osName.contains("linux") -> "linux-$osArch"
        osName.contains("mac") -> "macos-$osArch"
        osName.contains("windows") -> "windows-$osArch"
        java.io.File.separatorChar == '/' -> "unix-$osArch"
        else -> "unknown_$osArch"
    }
}
