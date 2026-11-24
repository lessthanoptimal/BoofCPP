package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import boofcv.alg.filter.derivative.GImageDerivativeOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.factory.filter.kernel.FactoryKernel;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.NativeConvolveImageChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;

/// The ConvolveImage checks shared with the other variants, single threaded. Strict's results must be identical to
/// BoofCV's
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeConvolveImage extends NativeConvolveImageChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeConvolveImage() {
        super(new NativeConvolveImage(), GrayU8.class, GrayF32.class);
    }

    /// Every operation and image type runs the _MT code with more than one thread
    @Test void concurrentToggle() {
        var hook = new NativeConvolveImage();
        for (Class type : new Class[]{GrayU8.class, GrayF32.class}) {
            ImageGray input = randomSubImage(type, 40, 30, 0, 100);
            ImageGray output = GeneralizedImageOps.createSingleBand(
                    GImageDerivativeOps.getDerivativeType(type), input.width, input.height);
            ImageBorder border = FactoryImageBorder.single(BorderType.REFLECT, type);
            Kernel1D kernel1 = FactoryKernel.random(FactoryKernel.getKernelType(type, 1), 2, -10, 10, rand);
            Kernel2D kernel2 = FactoryKernel.random(FactoryKernel.getKernelType(type, 2), 2, -10, 10, rand);
            assertConcurrentToggle(true, () -> hook.horizontal(kernel1, input, output, border));
            assertConcurrentToggle(true, () -> hook.vertical(kernel1, input, output, border));
            assertConcurrentToggle(true, () -> hook.convolve(kernel2, input, output, border));
        }
    }
}
