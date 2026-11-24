package org.boofcpp;

/// Controls the native thread pool, which every variant shares. BoofCV's BoofConcurrency doesn't change it.
///
/// Its native functions are in every variant's JNI library. It loads strict's, which every variant needs
public class NativeConcurrency {
    static {
        NativeLoader.load("JNIBoofCPP", "boofcpp-jni");
    }

    /// Most native threads a call uses, including the calling thread. 1 runs the single threaded code. Defaults to the
    /// number of cores the system reports
    public static int getMaxThreads() {
        return nativeGetMaxThreads();
    }

    /// Sets getMaxThreads(). Throws IllegalArgumentException if 'threads' is less than 1
    public static void setMaxThreads( int threads ) {
        nativeSetMaxThreads(threads);
    }

    /// For tests only: see setTestingBlocks()
    public static int getTestingBlocks() {
        return nativeGetTestingBlocks();
    }

    /// For tests only. If above 0, native loops are split into that many blocks whatever their amount of work, so the
    /// concurrent code also runs on small images. 0, the default, turns it off. Throws IllegalArgumentException if
    /// 'blocks' is negative
    public static void setTestingBlocks( int blocks ) {
        nativeSetTestingBlocks(blocks);
    }

    /// For tests only. Number of native loops split by setTestingBlocks() that the calling thread ran, so a test can
    /// tell whether a call ran concurrent code
    public static long getTestingLoops() {
        return nativeGetTestingLoops();
    }

    private static native int nativeGetMaxThreads();

    private static native void nativeSetMaxThreads( int threads );

    private static native int nativeGetTestingBlocks();

    private static native void nativeSetTestingBlocks( int blocks );

    private static native long nativeGetTestingLoops();
}
