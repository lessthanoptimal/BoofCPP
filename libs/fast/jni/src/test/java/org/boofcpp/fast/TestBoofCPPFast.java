package org.boofcpp.fast;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.override.BOverrideManager;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class TestBoofCPPFast extends BoofCPPStandardJUnit {
    @AfterEach void clearHooks() {
        BOverrideManager.clear();
    }

    /// Strict's initialize() doesn't register fast's hooks, even with fast on the classpath
    @Test void strictLeavesFastOut() {
        BoofCPPStrict.initialize();
        assertEquals(org.boofcpp.convolve.NativeImageBlurOps.class, BOverrideBlurImageOps.mean.getClass());
    }
}
