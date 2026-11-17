package org.boofcpp.threshold;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeLocalMeanChecks;

/// The local mean threshold checks shared with the other variants
public class TestNativeLocalMean extends NativeLocalMeanChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeLocalMean() {
        super(NativeLocalMean::new, GrayU8.class, GrayF32.class);
    }
}
