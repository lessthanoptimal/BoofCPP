package org.boofcpp.testing;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.BorderIndex1D_Extend;
import boofcv.core.image.border.BorderIndex1D_Wrap;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.border.ImageBorder1D_F32;
import boofcv.struct.border.ImageBorder1D_F64;
import boofcv.struct.border.ImageBorder1D_S32;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.List;
import java.util.function.Supplier;

import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's mean blur hook, without a border (BoofCV's mean) and with one (BoofCV's meanB), shared by
/// every variant. A variant's test class extends this, passes its hook and the image types it supports, and sets its
/// tolerance
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeMeanBlurChecks extends BoofCPPStandardJUnit {
    private final BOverrideBlurImageOps.Mean hook;
    private final Class[] imageTypes;

    protected NativeMeanBlurChecks( BOverrideBlurImageOps.Mean hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    /// Mean against BoofCV's Java code. Includes regions larger than the image
    @Test public void compareMean() {
        int[][] radii = new int[][]{{1, 1}, {2, 3}, {5, 4}, {24, 24}, {40, 2}, {3, 30}};

        for (Class type : imageTypes) {
            for (int[] r : radii) {
                // taller than BoofCV's VERTICAL_RESTART rows
                ImageGray input = randomSubImage(type, 61, 150, 0, maxValue(type));
                ImageGray expected = callBoofCV(null, () -> GBlurImageOps.mean(input, null, r[0], r[1], null));

                ImageGray found = (ImageGray)input.createSameShape();
                hook.processMean(input, found, r[0], r[1], BorderType.NORMALIZED, null);
                assertMatches(expected, found, maxValue(type));
            }
        }
    }

    /// The output is the input
    @Test public void compareMeanInPlace() {
        for (Class type : imageTypes) {
            ImageGray image = randomSubImage(type, 61, 45, 0, maxValue(type));
            ImageGray expected = callBoofCV(null, () -> GBlurImageOps.mean(image, null, 2, 3, null));
            hook.processMean(image, image, 2, 3, BorderType.NORMALIZED, null);
            assertMatches(expected, image, maxValue(type));
        }
    }

    /// A radius of zero or less is rejected
    @Test public void badRadius() {
        for (Class type : imageTypes) {
            ImageGray image = randomSubImage(type, 10, 8, 0, maxValue(type));
            ImageGray output = (ImageGray)image.createSameShape();
            for (BorderType border : new BorderType[]{BorderType.NORMALIZED, BorderType.REFLECT}) {
                assertThrows(IllegalArgumentException.class, () -> hook.processMean(image, output, 0, 2, border, null));
                assertThrows(IllegalArgumentException.class, () -> hook.processMean(image, output, 2, -1, border, null));
            }
        }
    }

    /// Types the variant doesn't support throw a RuntimeException, so BoofCV uses its Java code
    @Test public void unsupportedTypesThrow() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            for (BorderType border : new BorderType[]{BorderType.NORMALIZED, BorderType.EXTENDED}) {
                assertThrows(RuntimeException.class, () ->
                        hook.processMean(input, input.createSameShape(), 2, 2, border, null));
            }
        }
    }

    /// Every border against BoofCV's meanB. Floats include negative values, which make sums cancel
    @Test public void compareMeanWithBorder() {
        int[][] radii = new int[][]{{1, 1}, {2, 3}, {5, 4}, {12, 12}};

        for (Class type : imageTypes) {
            for (int[] r : radii) {
                for (BorderType border : BORDERS) {
                    ImageGray input = randomImage(type, 61, 150);
                    ImageGray expected = callBoofCV(null, () -> (ImageGray)GBlurImageOps.meanB(input, null, r[0], r[1],
                            FactoryImageBorder.single(border, type), null));

                    ImageGray found = (ImageGray)input.createSameShape();
                    hook.processMean(input, found, r[0], r[1], border, null);
                    assertMatches(expected, found, maxValue(type));
                }
            }
        }
    }

    /// Images smaller than the region, so every pixel is a border pixel
    @Test public void smallerThanRegionWithBorder() {
        for (Class type : imageTypes) {
            for (int[] shape : new int[][]{{1, 1}, {3, 2}, {2, 5}}) {
                for (BorderType border : BORDERS) {
                    ImageGray input = randomImage(type, shape[0], shape[1]);
                    ImageGray expected = callBoofCV(null, () -> (ImageGray)GBlurImageOps.meanB(input, null, 3, 3,
                            FactoryImageBorder.single(border, type), null));

                    ImageGray found = GeneralizedImageOps.createSingleBand(type, 1, 1);
                    ImageGray storage = GeneralizedImageOps.createSingleBand(type, 1, 1);
                    hook.processMean(input, found, 3, 3, border, storage);
                    assertMatches(expected, found, maxValue(type));
                }
            }
        }
    }

    /// meanB without a border throws, so BoofCV uses its Java code. So does meanB with a border BoofCV doesn't pass
    /// to the hook, e.g. rows and columns with different borders
    @Test public void unsupportedWithBorderFallsBackToJava() {
        for (Class type : imageTypes) {
            ImageGray input = randomImage(type, 20, 15);
            assertThrows(RuntimeException.class, () ->
                    hook.processMean(input, input.createSameShape(), 2, 2, BorderType.SKIP, null));
            for (ImageBorder border : Arrays.asList(null, mixedBorder(type))) {
                ImageGray expected = callBoofCV(null, () ->
                        (ImageGray)GBlurImageOps.meanB(input, null, 2, 2, border, null));
                ImageGray found = callBoofCV(hook, () ->
                        (ImageGray)GBlurImageOps.meanB(input, null, 2, 2, border, null));
                assertMatches(expected, found, maxValue(type));
            }
        }
    }

    /// Rows extended and columns wrapped
    private static ImageBorder mixedBorder( Class type ) {
        if (type == GrayF32.class)
            return new ImageBorder1D_F32(new BorderIndex1D_Extend(), new BorderIndex1D_Wrap());
        if (type == GrayF64.class)
            return new ImageBorder1D_F64(new BorderIndex1D_Extend(), new BorderIndex1D_Wrap());
        return new ImageBorder1D_S32(new BorderIndex1D_Extend(), new BorderIndex1D_Wrap());
    }

    /// Borders native can implement
    protected static final BorderType[] BORDERS =
            {BorderType.EXTENDED, BorderType.REFLECT, BorderType.WRAP, BorderType.ZERO};

    /// Random sub-image. Integers are in [0, maxValue], floats in [-maxValue, maxValue]
    private ImageGray randomImage( Class type, int width, int height ) {
        double max = maxValue(type);
        boolean isFloat = type == GrayF32.class || type == GrayF64.class;
        return randomSubImage(type, width, height, isFloat ? -max : 0, max);
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
