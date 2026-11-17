package org.boofcpp.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeBlockMinMaxChecks;

/// The block min-max threshold checks shared with the other variants
public class TestNativeBlockMinMax extends NativeBlockMinMaxChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeBlockMinMax() {
        super(NativeBlockMinMax::new, GrayU8.class, GrayF32.class);
    }
}
