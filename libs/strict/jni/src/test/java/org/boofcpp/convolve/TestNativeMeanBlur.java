package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeMeanBlurChecks;

/// The mean blur checks shared with the other variants. Strict's results must be identical to BoofCV's
public class TestNativeMeanBlur extends NativeMeanBlurChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeMeanBlur() {
        super(new NativeImageBlurOps(), GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class);
    }
}
