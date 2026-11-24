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

import java.util.ArrayList;
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
                ImageGray input = randomSubImage(type, 61, 45, 0, maxValue(type));
                ImageGray expected = callBoofCV(null, () -> GBlurImageOps.mean(input, null, r[0], r[1], null, null));

                ImageGray found = (ImageGray)input.createSameShape();
                hook.processMeanWeighted(input, found, r[0], r[1], null);
                assertMatches(expected, found, maxValue(type));
            }
        }
    }

    /// The output is the input
    @Test public void compareMeanInPlace() {
        for (Class type : imageTypes) {
            ImageGray image = randomSubImage(type, 61, 45, 0, maxValue(type));
            ImageGray expected = callBoofCV(null, () -> GBlurImageOps.mean(image, null, 2, 3, null, null));
            hook.processMeanWeighted(image, image, 2, 3, null);
            assertMatches(expected, image, maxValue(type));
        }
    }

    /// A radius of zero or less is rejected
    @Test public void badRadius() {
        for (Class type : imageTypes) {
            ImageGray image = randomSubImage(type, 10, 8, 0, maxValue(type));
            ImageGray output = (ImageGray)image.createSameShape();
            assertThrows(IllegalArgumentException.class, () -> hook.processMeanWeighted(image, output, 0, 2, null));
            assertThrows(IllegalArgumentException.class, () -> hook.processMeanWeighted(image, output, 2, -1, null));
        }
    }

    /// Types the variant doesn't support throw a RuntimeException, so BoofCV uses its Java code
    @Test public void unsupportedTypesThrow() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            assertThrows(RuntimeException.class, () ->
                    hook.processMeanWeighted(input, input.createSameShape(), 2, 2, null));
        }
    }

    /// Every border against BoofCV's meanB. Floats include negative values, which make sums cancel
    @Test public void compareMeanWithBorder() {
        int[][] radii = new int[][]{{1, 1}, {2, 3}, {5, 4}, {12, 12}};

        for (Class type : imageTypes) {
            for (int[] r : radii) {
                for (ImageBorder border : borders(type)) {
                    ImageGray input = randomImage(type, 61, 45);
                    ImageGray expected = callBoofCV(null, () ->
                            (ImageGray)GBlurImageOps.meanB(input, null, r[0], r[1], border, null, null));

                    ImageGray found = (ImageGray)input.createSameShape();
                    hook.processMeanBorder(input, found, r[0], r[1], border, null);
                    assertMatches(expected, found, maxValue(type));
                }
            }
        }
    }

    /// Images smaller than the region. BoofCV's meanB can't process these, so the result is compared to the mean
    /// computed from its definition: a horizontal mean through the border, then a vertical mean through the border.
    /// Every pixel is a border pixel here, where BoofCV adds up each pixel times 1/length for floats, and adds up the
    /// pixels and rounds the division for integers
    @Test public void smallerThanRegionWithBorder() {
        for (Class type : imageTypes) {
            for (int[] shape : new int[][]{{1, 1}, {3, 2}, {2, 5}}) {
                for (ImageBorder border : borders(type)) {
                    ImageGray input = randomImage(type, shape[0], shape[1]);
                    ImageGray found = GeneralizedImageOps.createSingleBand(type, 1, 1);
                    ImageGray storage = GeneralizedImageOps.createSingleBand(type, 1, 1);
                    hook.processMeanBorder(input, found, 3, 3, border, storage);

                    ImageGray horizontal = naiveMeanBorder(input, 3, 0, border);
                    assertMatches(naiveMeanBorder(horizontal, 0, 3, border), found,
                            maxValue(type));
                }
            }
        }
    }

    /// A radius of zero or less is rejected
    @Test public void badRadiusWithBorder() {
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            ImageBorder border = FactoryImageBorder.single(BorderType.REFLECT, type);
            assertThrows(IllegalArgumentException.class, () ->
                    hook.processMeanBorder(input, input.createSameShape(), 0, 2, border, null));
            assertThrows(IllegalArgumentException.class, () ->
                    hook.processMeanBorder(input, input.createSameShape(), 2, 0, border, null));
            assertThrows(IllegalArgumentException.class, () ->
                    hook.processMeanBorder(input, input.createSameShape(), -1, 2, border, null));
        }
    }

    /// Borders and types the variant doesn't support throw a RuntimeException, and BoofCV's meanB then uses its own
    /// code: no border, rows and columns with different borders, and integer values that don't fit the pixels
    @Test public void unsupportedWithBorderFallsBackToJava() {
        for (Class type : imageTypes) {
            ImageGray input = randomImage(type, 20, 15);
            List<ImageBorder> unsupported = new ArrayList<>();
            unsupported.add(mixedBorder(type));
            if (type == GrayU8.class || type == GrayU16.class) {
                unsupported.add(FactoryImageBorder.singleValue(type == GrayU8.class ? 300 : 70000, type));
                unsupported.add(FactoryImageBorder.singleValue(-1, type));
            }
            for (ImageBorder border : unsupported) {
                assertThrows(RuntimeException.class, () ->
                        hook.processMeanBorder(input, input.createSameShape(), 2, 2, border, null));
                ImageGray expected = callBoofCV(null, () ->
                        (ImageGray)GBlurImageOps.meanB(input, null, 2, 2, border, null, null));
                ImageGray found = callBoofCV(hook, () ->
                        (ImageGray)GBlurImageOps.meanB(input, null, 2, 2, border, null, null));
                assertMatches(expected, found, maxValue(type));
            }
            assertThrows(RuntimeException.class, () ->
                    hook.processMeanBorder(input, input.createSameShape(), 2, 2, null, null));
        }
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            ImageBorder border = FactoryImageBorder.single(BorderType.EXTENDED, type);
            assertThrows(RuntimeException.class, () ->
                    hook.processMeanBorder(input, input.createSameShape(), 2, 2, border, null));
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

    /// Extended, reflected, wrapped, zero and a constant value
    protected static List<ImageBorder> borders( Class type ) {
        List<ImageBorder> list = new ArrayList<>();
        for (BorderType b : new BorderType[]{BorderType.EXTENDED, BorderType.REFLECT, BorderType.WRAP, BorderType.ZERO})
            list.add(FactoryImageBorder.single(b, type));
        list.add(FactoryImageBorder.singleValue(type == GrayU8.class || type == GrayU16.class ? 7 : 7.5, type));
        return list;
    }

    /// Random sub-image. Integers are in [0, maxValue], floats in [-maxValue, maxValue]
    private ImageGray randomImage( Class type, int width, int height ) {
        double max = maxValue(type);
        boolean isFloat = type == GrayF32.class || type == GrayF64.class;
        return randomSubImage(type, width, height, isFloat ? -max : 0, max);
    }

    /// Mean along x over 2*radiusX+1 pixels, then along y over 2*radiusY+1 pixels, of every pixel read through the
    /// border. Same arithmetic as BoofCV's border code for a mean kernel
    private static ImageGray naiveMeanBorder( ImageGray input, int radiusX, int radiusY, ImageBorder border ) {
        ImageGray output = (ImageGray)input.createSameShape();
        border.setImage(input);
        int length = (2*radiusX + 1)*(2*radiusY + 1);
        for (int y = 0; y < input.height; y++) {
            for (int x = 0; x < input.width; x++) {
                if (input instanceof GrayF32) {
                    float weight = 1.0f/length;
                    float total = 0;
                    for (int j = -radiusY; j <= radiusY; j++)
                        for (int i = -radiusX; i <= radiusX; i++)
                            total += (float)BoofCPPTesting.borderValue(border, x + i, y + j)*weight;
                    ((GrayF32)output).set(x, y, total);
                } else if (input instanceof GrayF64) {
                    double weight = 1.0/length;
                    double total = 0;
                    for (int j = -radiusY; j <= radiusY; j++)
                        for (int i = -radiusX; i <= radiusX; i++)
                            total += BoofCPPTesting.borderValue(border, x + i, y + j)*weight;
                    ((GrayF64)output).set(x, y, total);
                } else {
                    int total = 0;
                    for (int j = -radiusY; j <= radiusY; j++)
                        for (int i = -radiusX; i <= radiusX; i++)
                            total += (int)BoofCPPTesting.borderValue(border, x + i, y + j);
                    GeneralizedImageOps.set(output, x, y, (total + length/2)/length);
                }
            }
        }
        return output;
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
