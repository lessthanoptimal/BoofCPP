package org.boofcpp.testing;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
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

    /// Mean against BoofCV's Java code. Includes regions larger than the image
    @Test
    public void compareMean() {
        int[][] radii = new int[][]{{1, 1}, {2, 3}, {5, 4}, {24, 24}, {40, 2}, {3, 30}};

        for (Class type : imageTypes) {
            for (int[] r : radii) {
                ImageGray input = randomSubImage(type, 61, 45, 0, maxValue(type));
                ImageGray expected = callBoofCV(null, () -> GBlurImageOps.mean(input, null, r[0], r[1], null, null));

                ImageGray found = (ImageGray)input.createSameShape();
                hook.processMeanWeighted(input, found, r[0], r[1], null);
                assertMatches(expected, found, maxValue(type));
            }
        }
    }

    /// The output is the input
    @Test
    public void compareMeanInPlace() {
        for (Class type : imageTypes) {
            ImageGray image = randomSubImage(type, 61, 45, 0, maxValue(type));
            ImageGray expected = callBoofCV(null, () -> GBlurImageOps.mean(image, null, 2, 3, null, null));
            hook.processMeanWeighted(image, image, 2, 3, null);
            assertMatches(expected, image, maxValue(type));
        }
    }

    @Test
    public void badRadius() {
        for (Class type : imageTypes) {
            ImageGray image = randomSubImage(type, 10, 8, 0, maxValue(type));
            ImageGray output = (ImageGray)image.createSameShape();
            assertThrows(IllegalArgumentException.class, () -> hook.processMeanWeighted(image, output, 0, 2, null));
            assertThrows(IllegalArgumentException.class, () -> hook.processMeanWeighted(image, output, 2, -1, null));
        }
    }

    /// Types the variant doesn't support throw a RuntimeException, so BoofCV uses its Java code
    @Test
    public void unsupportedTypesThrow() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            assertThrows(RuntimeException.class, () ->
                    hook.processMeanWeighted(input, input.createSameShape(), 2, 2, null));
        }
    }

    /// Largest input value. It's the tolerance's scale because the mean is a running sum of raw pixel values, so
    /// floating point rounding error grows with their range
    protected static double maxValue( Class type ) {
        return type == GrayU16.class ? 65535 : 255;
    }

    /// Calls BoofCV single threaded with 'hook' as its mean blur, or with its Java code if 'hook' is null
    protected static <R> R callBoofCV( BOverrideBlurImageOps.Mean hook, Supplier<R> call ) {
        BOverrideBlurImageOps.Mean previous = BOverrideBlurImageOps.mean;
        try {
            BOverrideBlurImageOps.mean = hook;
            return BoofCPPTesting.singleThreaded(call);
        } finally {
            BOverrideBlurImageOps.mean = previous;
        }
    }
}
