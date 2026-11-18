package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.alg.misc.GImageMiscOps;
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

import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's mean blur with a border (BoofCV's meanB), shared by every variant. A variant's test class
/// extends this, passes its hook and the image types it supports, and sets its tolerance
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeMeanBorderChecks extends BoofCPPStandardJUnit {
    private final BOverrideBlurImageOps.Mean hook;
    private final Class[] imageTypes;

    protected NativeMeanBorderChecks( BOverrideBlurImageOps.Mean hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    /// Every border against BoofCV's meanB. Floats include negative values, which make sums cancel
    @Test
    public void compareMeanBorder() {
        int[][] radii = new int[][]{{1, 1}, {2, 3}, {5, 4}, {12, 12}};

        for (Class type : imageTypes) {
            for (int[] r : radii) {
                for (ImageBorder border : borders(type)) {
                    ImageGray input = randomImage(type, 61, 45);
                    ImageGray expected = NativeMeanBlurChecks.callBoofCV(null, () ->
                            (ImageGray)GBlurImageOps.meanB(input, null, r[0], r[1], border, null, null));

                    ImageGray found = (ImageGray)input.createSameShape();
                    hook.processMeanBorder(input, found, r[0], r[1], border, null);
                    assertMatches(expected, found, NativeMeanBlurChecks.maxValue(type));
                }
            }
        }
    }

    /// Images smaller than the region. BoofCV's meanB can't process these, so the result is compared to the mean
    /// computed from its definition: a horizontal mean through the border, then a vertical mean through the border.
    /// Every pixel is a border pixel here, where BoofCV adds up each pixel times 1/length for floats, and adds up the
    /// pixels and rounds the division for integers
    @Test
    public void smallerThanRegion() {
        for (Class type : imageTypes) {
            for (int[] shape : new int[][]{{1, 1}, {3, 2}, {2, 5}}) {
                for (ImageBorder border : borders(type)) {
                    ImageGray input = randomImage(type, shape[0], shape[1]);
                    ImageGray found = GeneralizedImageOps.createSingleBand(type, 1, 1);
                    ImageGray storage = GeneralizedImageOps.createSingleBand(type, 1, 1);
                    hook.processMeanBorder(input, found, 3, 3, border, storage);

                    ImageGray horizontal = naiveMeanBorder(input, 3, 0, border);
                    assertMatches(naiveMeanBorder(horizontal, 0, 3, border), found,
                            NativeMeanBlurChecks.maxValue(type));
                }
            }
        }
    }

    @Test
    public void badRadius() {
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            ImageBorder border = FactoryImageBorder.single(BorderType.REFLECT, type);
            assertThrows(IllegalArgumentException.class, () ->
                    hook.processMeanBorder(input, input.createSameShape(), 0, 2, border, null));
            assertThrows(IllegalArgumentException.class, () ->
                    hook.processMeanBorder(input, input.createSameShape(), 2, 0, border, null));
        }
    }

    /// Borders and types the variant doesn't support throw a RuntimeException, and BoofCV's meanB then uses its own
    /// code: no border, rows and columns with different borders, and integer values that don't fit the pixels
    @Test
    public void unsupportedFallsBackToJava() {
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
                ImageGray expected = NativeMeanBlurChecks.callBoofCV(null, () ->
                        (ImageGray)GBlurImageOps.meanB(input, null, 2, 2, border, null, null));
                ImageGray found = NativeMeanBlurChecks.callBoofCV(hook, () ->
                        (ImageGray)GBlurImageOps.meanB(input, null, 2, 2, border, null, null));
                assertMatches(expected, found, NativeMeanBlurChecks.maxValue(type));
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
        ImageGray input = GeneralizedImageOps.createSingleBand(type, width, height);
        double max = NativeMeanBlurChecks.maxValue(type);
        boolean isFloat = type == GrayF32.class || type == GrayF64.class;
        GImageMiscOps.fillUniform(input, rand, isFloat ? -max : 0, max);
        return BoofTesting.createSubImageOf(input);
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
                            total += (float)get(border, x + i, y + j)*weight;
                    ((GrayF32)output).set(x, y, total);
                } else if (input instanceof GrayF64) {
                    double weight = 1.0/length;
                    double total = 0;
                    for (int j = -radiusY; j <= radiusY; j++)
                        for (int i = -radiusX; i <= radiusX; i++)
                            total += get(border, x + i, y + j)*weight;
                    ((GrayF64)output).set(x, y, total);
                } else {
                    int total = 0;
                    for (int j = -radiusY; j <= radiusY; j++)
                        for (int i = -radiusX; i <= radiusX; i++)
                            total += (int)get(border, x + i, y + j);
                    GeneralizedImageOps.set(output, x, y, (total + length/2)/length);
                }
            }
        }
        return output;
    }

    private static double get( ImageBorder border, int x, int y ) {
        double[] pixel = new double[1];
        border.getGeneral(x, y, pixel);
        return pixel[0];
    }
}
