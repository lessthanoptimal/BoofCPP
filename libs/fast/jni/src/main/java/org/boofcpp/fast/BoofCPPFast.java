package org.boofcpp.fast;

import org.boofcpp.NativeLoader;

/// Loads fast's native library
public class BoofCPPFast {
    private static final boolean loaded;

    static {
        loaded = NativeLoader.load("JNIBoofCPPFast", "boofcpp-fast-jni");
    }

    /// True if the native library is loaded
    public static boolean isLoaded() {
        return loaded;
    }

    /// Compiler, build type and compiler flags of the native library
    public static native String nativeBuildInfo();
}
