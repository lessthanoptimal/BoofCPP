package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.ImageGray;
import org.junit.jupiter.api.Test;

import java.util.function.Supplier;

import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's mean blur hook, shared by every variant. A variant's test class extends this, passes its
/// hook and the image types it supports, and sets its tolerance
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeMeanBlurChecks extends BoofCPPStandardJUnit {
    private final BOverrideBlurImageOps.Mean hook;
    private final Class[] imageTypes;

    protected NativeMeanBlurChecks( BOverrideBlurImageOps.Mean hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    /// Mean against BoofCV's Java code, called directly and through BoofCV's hook. Includes regions larger than the
    /// image
    @Test
    public void compareMean() {
        int[][] radii = new int[][]{{1, 1}, {2, 3}, {5, 4}, {24, 24}, {40, 2}, {3, 30}};

        for (Class type : imageTypes) {
            for (boolean concurrent : new boolean[]{false, true}) {
                for (int[] r : radii) {
                    ImageGray input = randomImage(type, 61, 45);
                    double scale = maxValue(type);
                    ImageGray expected = callBoofCV(null, concurrent,
                            () -> GBlurImageOps.mean(input, null, r[0], r[1], null, null));

                    ImageGray found = (ImageGray)input.createSameShape();
                    hook.processMeanWeighted(input, found, r[0], r[1], null);
                    assertMatches(expected, found, scale);

                    assertMatches(expected, callBoofCV(hook, concurrent,
                            () -> GBlurImageOps.mean(input, null, r[0], r[1], null, null)), scale);
                }
            }
        }
    }

    /// The output is the input
    @Test
    public void compareMeanInPlace() {
        for (Class type : imageTypes) {
            ImageGray image = randomImage(type, 61, 45);
            ImageGray expected = callBoofCV(null, false, () -> GBlurImageOps.mean(image, null, 2, 3, null, null));
            hook.processMeanWeighted(image, image, 2, 3, null);
            assertMatches(expected, image, maxValue(type));
        }
    }

    @Test
    public void badRadius() {
        for (Class type : imageTypes) {
            ImageGray image = randomImage(type, 10, 8);
            ImageGray output = (ImageGray)image.createSameShape();
            assertThrows(IllegalArgumentException.class, () -> hook.processMeanWeighted(image, output, 0, 2, null));
            assertThrows(IllegalArgumentException.class, () -> hook.processMeanWeighted(image, output, 2, -1, null));
        }
    }

    /// Largest input value. It's the tolerance's scale because the mean is a running sum of raw pixel values, so
    /// floating point rounding error grows with their range
    protected static double maxValue( Class type ) {
        return type == GrayU16.class ? 65535 : 255;
    }

    /// Random sub-image, so that its offset and stride aren't the defaults
    protected ImageGray randomImage( Class type, int width, int height ) {
        ImageGray input = GeneralizedImageOps.createSingleBand(type, width, height);
        GImageMiscOps.fillUniform(input, rand, 0, maxValue(type));
        return BoofTesting.createSubImageOf(input);
    }

    /// Calls BoofCV with 'hook' as its mean blur, or with its Java code if 'hook' is null
    protected static <R> R callBoofCV( BOverrideBlurImageOps.Mean hook, boolean concurrent, Supplier<R> call ) {
        BOverrideBlurImageOps.Mean previous = BOverrideBlurImageOps.mean;
        boolean previousConcurrent = BoofConcurrency.USE_CONCURRENT;
        try {
            BOverrideBlurImageOps.mean = hook;
            BoofConcurrency.USE_CONCURRENT = concurrent;
            return call.get();
        } finally {
            BOverrideBlurImageOps.mean = previous;
            BoofConcurrency.USE_CONCURRENT = previousConcurrent;
        }
    }
}
