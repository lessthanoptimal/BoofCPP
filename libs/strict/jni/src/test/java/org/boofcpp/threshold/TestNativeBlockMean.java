package org.boofcpp.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeBlockMeanChecks;

/// The block mean threshold checks shared with the other variants
public class TestNativeBlockMean extends NativeBlockMeanChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeBlockMean() {
        super(NativeBlockMean::new, GrayU8.class, GrayF32.class);
    }
}
