package org.boofcpp.disparity;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeDisparityBlockMatchChecks;

/// The block matching checks shared with the other variants
public class TestNativeDisparityBlockMatch extends NativeDisparityBlockMatchChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeDisparityBlockMatch() {
        super(NativeDisparityBlockMatch::create, GrayU8.class, GrayF32.class);
    }
}
