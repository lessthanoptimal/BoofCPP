package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.testing.NativeGaussianBlurChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;

/// The Gaussian blur checks shared with the other variants, single threaded. Strict's results must be identical to
/// BoofCV's
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeGaussianBlur extends NativeGaussianBlurChecks {
    static {
        BoofCPPStrict.loadlib();
    }

    public TestNativeGaussianBlur() {
        super(new NativeImageBlurOps(), GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class);
    }

    /// Runs the _MT code with more than one thread
    @Test void concurrentToggle() {
        var hook = new NativeImageBlurOps();
        ImageGray input = randomSubImage(GrayU8.class, 40, 30, 0, 100);
        ImageGray output = (ImageGray)input.createSameShape();
        assertConcurrentToggle(true, () -> hook.processGaussian(input, output, -1, 2, -1, 3, null));
    }
}
