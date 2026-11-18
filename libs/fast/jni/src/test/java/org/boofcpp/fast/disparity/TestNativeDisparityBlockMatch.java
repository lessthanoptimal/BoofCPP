package org.boofcpp.fast.disparity;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeDisparityBlockMatchChecks;
import org.boofcpp.testing.Tolerance;

/// The block matching checks shared with the other variants
public class TestNativeDisparityBlockMatch extends NativeDisparityBlockMatchChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeDisparityBlockMatch() {
        super(NativeDisparityBlockMatch::create, GrayU8.class, GrayF32.class);
        tolerance = Tolerance.FLOAT;
    }
}
