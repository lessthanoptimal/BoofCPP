package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeConvolveNormalizedChecks;

/// The normalized convolution checks shared with the other variants. Strict's results must be identical to BoofCV's
public class TestNativeImageConvolveNormalized extends NativeConvolveNormalizedChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeImageConvolveNormalized() {
        super(new NativeImageConvolveNormalized(), GrayU8.class, GrayF32.class);
    }
}
