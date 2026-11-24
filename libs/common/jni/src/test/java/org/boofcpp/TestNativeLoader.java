package org.boofcpp;

import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertFalse;

public class TestNativeLoader extends BoofCPPStandardJUnit {
    /// A library which isn't anywhere fails without throwing
    @Test public void missing() {
        assertFalse(NativeLoader.load("JNIBoofCPPMissing", "boofcpp-missing-jni"));
    }
}
