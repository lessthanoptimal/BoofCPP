package org.boofcpp.fast;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import org.boofcpp.NativeLoader;
import org.boofcpp.fast.convolve.NativeImageBlurOps;

/// Fast: BoofCPP's native code with results within floating point tolerance of BoofCV's Java code
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

    /// Registers fast's hooks with BoofCV. It only has some functions, and some image types of those, so register the
    /// hooks for everything else first, e.g. with BoofCPPStrict.initialize(). Fast passes what it doesn't support to
    /// the hook it replaced, or to BoofCV's Java code if there was none. Throws if its native library didn't load
    @SuppressWarnings({"rawtypes", "unchecked"})
    public static void initialize() {
        if (!loaded)
            throw new RuntimeException("Fast's native library didn't load");
        BOverrideBlurImageOps.mean = new NativeImageBlurOps(BOverrideBlurImageOps.mean);
    }
}
