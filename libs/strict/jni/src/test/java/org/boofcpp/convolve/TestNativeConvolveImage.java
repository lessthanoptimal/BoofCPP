package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPPStrict;
import boofcv.factory.filter.kernel.FactoryKernel;
import boofcv.struct.border.BorderType;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.convolve.Kernel2D_S32;
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
        BoofCPPStrict.loadlib();
    }

    public TestNativeConvolveImage() {
        super(new NativeConvolveImage(), GrayU8.class, GrayF32.class);
    }

    /// Every operation runs the _MT code with more than one thread
    @Test void concurrentToggle() {
        var hook = new NativeConvolveImage();
        ImageGray input = randomSubImage(GrayU8.class, 40, 30, 0, 100);
        var output = new GrayS16(input.width, input.height);
        Kernel1D kernel1 = FactoryKernel.random(Kernel1D_S32.class, 2, -10, 10, rand);
        Kernel2D kernel2 = FactoryKernel.random(Kernel2D_S32.class, 2, -10, 10, rand);
        assertConcurrentToggle(true, () -> hook.horizontal(kernel1, input, output, BorderType.REFLECT));
        assertConcurrentToggle(true, () -> hook.vertical(kernel1, input, output, BorderType.REFLECT));
        assertConcurrentToggle(true, () -> hook.convolve(kernel2, input, output, BorderType.REFLECT));
    }
}
