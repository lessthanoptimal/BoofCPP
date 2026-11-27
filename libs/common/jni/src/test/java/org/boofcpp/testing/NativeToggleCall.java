package org.boofcpp.testing;

import org.boofcpp.NativeLoader;

/// A native call with a concurrent switch, only for tests of BoofCPPTesting.assertConcurrentToggle()
public class NativeToggleCall {
    static {
        if (!NativeLoader.load("JNIBoofCPPCommonTest", "boofcpp-common-test"))
            throw new RuntimeException("The test library JNIBoofCPPCommonTest didn't load. Build the native code first");
    }

    /// Runs a concurrent loop with more than one thread, otherwise nothing
    public static native void call();
}
