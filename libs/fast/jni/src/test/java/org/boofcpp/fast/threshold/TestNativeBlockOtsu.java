package org.boofcpp.fast.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeBlockOtsuChecks;

/// The block Otsu threshold checks shared with the other variants. Binary images, so fast's results are identical
public class TestNativeBlockOtsu extends NativeBlockOtsuChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeBlockOtsu() {
        super(NativeBlockOtsu::new, GrayU8.class, GrayF32.class);
    }
}
