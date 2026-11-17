package org.boofcpp.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeGlobalOtsuChecks;

/// The global otsu threshold checks shared with the other variants
public class TestNativeGlobalOtsu extends NativeGlobalOtsuChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeGlobalOtsu() {
        super(NativeGlobalOtsu::new, GrayU8.class, GrayF32.class);
    }
}
