package org.boofcpp.convolve;

import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeConvolveNormalizedChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;

/// The normalized convolution checks shared with the other variants, single threaded. Strict's results must be
/// identical to BoofCV's
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeImageConvolveNormalized extends NativeConvolveNormalizedChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeImageConvolveNormalized() {
        super(new NativeImageConvolveNormalized(), GrayU8.class, GrayF32.class);
    }

    /// Every operation runs the _MT code with more than one thread
    @Test void concurrentToggle() {
        var hook = new NativeImageConvolveNormalized();
        ImageGray input = randomSubImage(GrayU8.class, 40, 30, 0, 100);
        ImageGray output = (ImageGray)input.createSameShape();
        Kernel1D kernel1 = FactoryKernelGaussian.gaussian1D(GrayU8.class, -1, 2);
        Kernel2D kernel2 = FactoryKernelGaussian.gaussian2D(GrayU8.class, -1, 2);
        assertConcurrentToggle(true, () -> hook.horizontal(kernel1, input, output));
        assertConcurrentToggle(true, () -> hook.vertical(kernel1, input, output));
        assertConcurrentToggle(true, () -> hook.convolve(kernel2, input, output));
    }
}
