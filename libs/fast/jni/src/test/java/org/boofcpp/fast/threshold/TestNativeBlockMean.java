package org.boofcpp.fast.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeBlockMeanChecks;

/// The block mean threshold checks shared with the other variants. Binary images, so fast's results are identical
public class TestNativeBlockMean extends NativeBlockMeanChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeBlockMean() {
        super(NativeBlockMean::new, GrayU8.class, GrayF32.class);
    }
}
