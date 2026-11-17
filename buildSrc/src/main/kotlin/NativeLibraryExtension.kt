import org.gradle.api.provider.Property

/// The JNI library of a variant, built by CMake into `<native build>/<directory>`
abstract class NativeLibraryExtension {
    /// Base name of the library, e.g. JNIBoofCPP for libJNIBoofCPP.so
    abstract val name: Property<String>

    /// Directory inside the native build directory
    abstract val directory: Property<String>
}
