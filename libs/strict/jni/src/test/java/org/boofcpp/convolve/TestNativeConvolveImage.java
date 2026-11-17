package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeConvolveImageChecks;

/// The ConvolveImage checks shared with the other variants. Strict's results must be identical to BoofCV's
public class TestNativeConvolveImage extends NativeConvolveImageChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeConvolveImage() {
        super(new NativeConvolveImage(), GrayU8.class, GrayF32.class);
    }
}
