package org.boofcpp.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeGlobalFixedChecks;

/// The global fixed threshold checks shared with the other variants
public class TestNativeGlobalFixed extends NativeGlobalFixedChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeGlobalFixed() {
        super(NativeGlobalFixed::new, GrayU8.class, GrayF32.class);
    }
}
