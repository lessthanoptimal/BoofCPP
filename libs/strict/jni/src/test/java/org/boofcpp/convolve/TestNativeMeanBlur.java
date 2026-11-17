package org.boofcpp.convolve;

import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeMeanBlurChecks;

/// The mean blur checks shared with the other variants. Strict's results must be identical to BoofCV's
public class TestNativeMeanBlur extends NativeMeanBlurChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeMeanBlur() {
        super(new NativeImageBlurOps(), TestNativeImageBlurOps.GRAY_TYPES);
    }
}
