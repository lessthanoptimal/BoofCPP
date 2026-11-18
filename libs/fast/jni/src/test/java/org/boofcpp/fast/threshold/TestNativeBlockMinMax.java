package org.boofcpp.fast.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeBlockMinMaxChecks;

/// The block min-max threshold checks shared with the other variants. Binary images, so fast's results are identical
public class TestNativeBlockMinMax extends NativeBlockMinMaxChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeBlockMinMax() {
        super(NativeBlockMinMax::new, GrayU8.class, GrayF32.class);
    }
}
