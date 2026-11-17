package org.boofcpp.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeBlockOtsuChecks;

/// The block otsu threshold checks shared with the other variants
public class TestNativeBlockOtsu extends NativeBlockOtsuChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeBlockOtsu() {
        super(NativeBlockOtsu::new, GrayU8.class, GrayF32.class);
    }
}
