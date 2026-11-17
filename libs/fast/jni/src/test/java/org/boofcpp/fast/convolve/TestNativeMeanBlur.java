package org.boofcpp.fast.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeMeanBlurChecks;
import org.boofcpp.testing.Tolerance;

/// The mean blur checks shared with the other variants. Fast supports GrayF32 and GrayF64
public class TestNativeMeanBlur extends NativeMeanBlurChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeMeanBlur() {
        super(new NativeImageBlurOps<>(null), GrayF32.class, GrayF64.class);
        tolerance = Tolerance.FLOAT;
    }
}
