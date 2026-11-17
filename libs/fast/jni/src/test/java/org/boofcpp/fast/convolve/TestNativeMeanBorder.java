package org.boofcpp.fast.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeMeanBorderChecks;
import org.boofcpp.testing.Tolerance;

/// The mean blur with a border checks shared with the other variants, within floating point tolerance of BoofCV
public class TestNativeMeanBorder extends NativeMeanBorderChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeMeanBorder() {
        super(new NativeImageBlurOps<>(null), GrayF32.class, GrayF64.class);
        tolerance = Tolerance.FLOAT;
    }
}
