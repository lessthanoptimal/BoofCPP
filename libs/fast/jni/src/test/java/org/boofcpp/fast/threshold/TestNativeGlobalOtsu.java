package org.boofcpp.fast.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeGlobalOtsuChecks;

/// The global Otsu threshold checks shared with the other variants. Binary images, so fast's results are identical
public class TestNativeGlobalOtsu extends NativeGlobalOtsuChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeGlobalOtsu() {
        super(NativeGlobalOtsu::new, GrayU8.class, GrayF32.class);
    }
}
