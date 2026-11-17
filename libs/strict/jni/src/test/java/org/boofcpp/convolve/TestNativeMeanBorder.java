package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeMeanBorderChecks;

/// The mean blur with a border checks shared with the other variants. Strict's results must be identical to BoofCV's
public class TestNativeMeanBorder extends NativeMeanBorderChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeMeanBorder() {
        super(new NativeImageBlurOps(), GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class);
    }
}
