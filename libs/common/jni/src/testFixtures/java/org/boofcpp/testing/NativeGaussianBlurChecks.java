package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.InterleavedU8;
import org.junit.jupiter.api.Test;

import java.util.function.Supplier;

import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's Gaussian blur hook against BoofCV's Java code, shared by every variant. A variant's test
/// class extends this, passes its hook and the image types it supports, and sets its tolerance
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeGaussianBlurChecks extends BoofCPPStandardJUnit {
    private final BOverrideBlurImageOps.Gaussian hook;
    private final Class[] imageTypes;

    protected NativeGaussianBlurChecks( BOverrideBlurImageOps.Gaussian hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    /// Against BoofCV's Java code. Sigma or radius can be computed from the other, and the kernel can be larger than
    /// the image
    @Test
    public void compareGaussian() {
        double[][] params = new double[][]{
                {-1, 1, -1, 1}, {-1, 2, -1, 5}, {1.5, -1, 1.5, -1}, {2.0, -1, 0.7, 3}, {-1, 12, 3.0, -1}};

        for (Class type : imageTypes) {
            for (int[] shape : new int[][]{{61, 45}, {6, 7}}) {
                for (double[] p : params) {
                    double sigmaX = p[0], sigmaY = p[2];
                    int radiusX = (int)p[1], radiusY = (int)p[3];
                    ImageGray input = randomImage(type, shape[0], shape[1]);
                    ImageGray expected = callBoofCV(null, () ->
                            GBlurImageOps.gaussian(input, null, sigmaX, radiusX, sigmaY, radiusY, null));

                    ImageGray found = (ImageGray)input.createSameShape();
                    hook.processGaussian(input, found, sigmaX, radiusX, sigmaY, radiusY, null);
                    assertMatches(expected, found, NativeMeanBlurChecks.maxValue(type));
                }
            }
        }
    }

    /// Images the variant doesn't support throw a RuntimeException, and BoofCV then uses its own code
    @Test
    public void unsupportedImagesFallBackToJava() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            assertThrows(RuntimeException.class, () ->
                    hook.processGaussian(input, input.createSameShape(), -1, 2, -1, 2, null));
        }

        InterleavedU8 input = new InterleavedU8(20, 15, 3);
        GImageMiscOps.fillUniform(input, rand, 0, 255);
        assertThrows(RuntimeException.class, () ->
                hook.processGaussian(input, new InterleavedU8(1, 1, 3), -1, 2, -1, 2, null));
        InterleavedU8 expected = callBoofCV(null, () -> GBlurImageOps.gaussian(input, null, -1, 2, -1, 3, null));
        InterleavedU8 found = callBoofCV(hook, () -> GBlurImageOps.gaussian(input, null, -1, 2, -1, 3, null));
        BoofTesting.assertEquals(expected, found, 0);
    }

    /// Random sub-image, so that its offset and stride aren't the defaults
    private ImageGray randomImage( Class type, int width, int height ) {
        ImageGray input = GeneralizedImageOps.createSingleBand(type, width, height);
        GImageMiscOps.fillUniform(input, rand, 0, NativeMeanBlurChecks.maxValue(type));
        return BoofTesting.createSubImageOf(input);
    }

    /// Calls BoofCV single threaded with 'hook' as its Gaussian blur, or with its Java code if 'hook' is null
    private static <R> R callBoofCV( BOverrideBlurImageOps.Gaussian hook, Supplier<R> call ) {
        BOverrideBlurImageOps.Gaussian previous = BOverrideBlurImageOps.gaussian;
        boolean previousConcurrent = BoofConcurrency.USE_CONCURRENT;
        try {
            BOverrideBlurImageOps.gaussian = hook;
            BoofConcurrency.USE_CONCURRENT = false;
            return call.get();
        } finally {
            BOverrideBlurImageOps.gaussian = previous;
            BoofConcurrency.USE_CONCURRENT = previousConcurrent;
        }
    }
}
