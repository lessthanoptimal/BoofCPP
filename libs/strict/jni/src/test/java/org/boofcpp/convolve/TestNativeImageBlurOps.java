package org.boofcpp.convolve;

import boofcv.BoofTesting;
import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.BorderIndex1D_Extend;
import boofcv.core.image.border.BorderIndex1D_Wrap;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.border.ImageBorder1D_S32;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.InterleavedU8;
import org.boofcpp.BoofCPP;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.function.Supplier;

import static org.boofcpp.testing.BoofCPPTesting.assertIdentical;
import static org.junit.jupiter.api.Assertions.*;

public class TestNativeImageBlurOps {
    Random rand = new Random(23423);

    static {
        BoofCPP.loadlib();
    }

    /// Image types BoofCV's BlurImageOps has mean and gaussian for
    static final Class[] GRAY_TYPES = new Class[]{GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class};

    /// Mean against BoofCV's Java code, called directly and through BoofCV's hook. Includes regions larger than the
    /// image
    @Test
    public void compareMean() {
        int[][] radii = new int[][]{{1, 1}, {2, 3}, {5, 4}, {24, 24}, {40, 2}, {3, 30}};

        for (Class type : GRAY_TYPES) {
            for (boolean concurrent : new boolean[]{false, true}) {
                for (int[] r : radii) {
                    ImageGray input = randomImage(type, 61, 45);
                    ImageGray expected = callBoofCV(false, concurrent,
                            () -> GBlurImageOps.mean(input, null, r[0], r[1], null, null));

                    ImageGray found = (ImageGray)input.createSameShape();
                    new NativeImageBlurOps().processMeanWeighted(input, found, r[0], r[1], null);
                    assertIdentical(expected, found);

                    assertIdentical(expected, callBoofCV(true, concurrent,
                            () -> GBlurImageOps.mean(input, null, r[0], r[1], null, null)));
                }
            }
        }
    }

    /// Gaussian against BoofCV's Java code, called directly and through BoofCV's hook. Sigma or radius can be computed
    /// from the other, and the kernel can be larger than the image
    @Test
    public void compareGaussian() {
        double[][] params = new double[][]{
                {-1, 1, -1, 1}, {-1, 2, -1, 5}, {1.5, -1, 1.5, -1}, {2.0, -1, 0.7, 3}, {-1, 12, 3.0, -1}};

        for (Class type : GRAY_TYPES) {
            for (int[] shape : new int[][]{{61, 45}, {6, 7}}) {
                for (boolean concurrent : new boolean[]{false, true}) {
                    for (double[] p : params) {
                        double sigmaX = p[0], sigmaY = p[2];
                        int radiusX = (int)p[1], radiusY = (int)p[3];
                        ImageGray input = randomImage(type, shape[0], shape[1]);
                        ImageGray expected = callBoofCV(false, concurrent, () ->
                                GBlurImageOps.gaussian(input, null, sigmaX, radiusX, sigmaY, radiusY, null));

                        ImageGray found = (ImageGray)input.createSameShape();
                        new NativeImageBlurOps().processGaussian(
                                input, found, sigmaX, radiusX, sigmaY, radiusY, null);
                        assertIdentical(expected, found);

                        assertIdentical(expected, callBoofCV(true, concurrent, () ->
                                GBlurImageOps.gaussian(input, null, sigmaX, radiusX, sigmaY, radiusY, null)));
                    }
                }
            }
        }
    }

    /// Native throws a RuntimeException for images it doesn't support, and BoofCV then uses its own code
    @Test
    public void meanGaussian_unsupportedFallsBackToJava() {
        NativeImageBlurOps nativeBlur = new NativeImageBlurOps();

        // BlurImageOps has no mean for GrayS16, but the hook's interface allows it
        GrayS16 inputS16 = new GrayS16(20, 15);
        assertThrows(RuntimeException.class,
                () -> nativeBlur.processMeanWeighted(inputS16, new GrayS16(1, 1), 2, 2, null));
        assertThrows(RuntimeException.class,
                () -> nativeBlur.processGaussian(inputS16, new GrayS16(1, 1), -1, 2, -1, 2, null));

        InterleavedU8 input = new InterleavedU8(20, 15, 3);
        GImageMiscOps.fillUniform(input, rand, 0, 255);
        assertThrows(RuntimeException.class,
                () -> nativeBlur.processGaussian(input, new InterleavedU8(1, 1, 3), -1, 2, -1, 2, null));

        InterleavedU8 expected = callBoofCV(false, false,
                () -> GBlurImageOps.gaussian(input, null, -1, 2, -1, 3, null));
        InterleavedU8 found = callBoofCV(true, false,
                () -> GBlurImageOps.gaussian(input, null, -1, 2, -1, 3, null));
        BoofTesting.assertEquals(expected, found, 0);
    }

    @Test
    public void mean_badRadius() {
        GrayU8 input = new GrayU8(20, 15);
        NativeImageBlurOps nativeBlur = new NativeImageBlurOps();
        assertThrows(IllegalArgumentException.class,
                () -> nativeBlur.processMeanWeighted(input, new GrayU8(1, 1), 0, 2, null));
        assertThrows(IllegalArgumentException.class,
                () -> nativeBlur.processMeanWeighted(input, new GrayU8(1, 1), 2, 0, null));
    }

    /// Random sub-image, so that its offset and stride aren't the defaults
    ImageGray randomImage( Class type, int width, int height ) {
        ImageGray input = GeneralizedImageOps.createSingleBand(type, width, height);
        GImageMiscOps.fillUniform(input, rand, 0, type == GrayU16.class ? 65535 : 255);
        return BoofTesting.createSubImageOf(input);
    }

    /// Calls BoofCV with BoofCPP's mean and gaussian hooks, or without them
    static <R> R callBoofCV( boolean useNative, boolean concurrent, Supplier<R> call ) {
        BOverrideBlurImageOps.Mean previousMean = BOverrideBlurImageOps.mean;
        BOverrideBlurImageOps.Gaussian previousGaussian = BOverrideBlurImageOps.gaussian;
        boolean previousConcurrent = BoofConcurrency.USE_CONCURRENT;
        try {
            BOverrideBlurImageOps.mean = useNative ? new NativeImageBlurOps() : null;
            BOverrideBlurImageOps.gaussian = useNative ? new NativeImageBlurOps() : null;
            BoofConcurrency.USE_CONCURRENT = concurrent;
            return call.get();
        } finally {
            BOverrideBlurImageOps.mean = previousMean;
            BOverrideBlurImageOps.gaussian = previousGaussian;
            BoofConcurrency.USE_CONCURRENT = previousConcurrent;
        }
    }

    /// Every image type that native meanB supports, with every border native supports, against BoofCV's meanB. The
    /// result must be identical when called directly and when BoofCV calls it through its hook
    @Test
    public void compareMeanBorder() {
        Class[] types = new Class[]{GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class};
        int[][] radii = new int[][]{{1, 1}, {2, 3}, {5, 4}, {12, 12}};

        for (Class type : types) {
            for (boolean concurrent : new boolean[]{false, true}) {
                for (int[] r : radii) {
                    for (ImageBorder border : supportedBorders(type)) {
                        ImageGray input = GeneralizedImageOps.createSingleBand(type, 61, 45);
                        GImageMiscOps.fillUniform(input, rand, 0, type == GrayU16.class ? 65535 : 255);
                        input = BoofTesting.createSubImageOf(input);
                        compareMeanBorder(input, r[0], r[1], border, concurrent);
                    }
                }
            }
        }
    }

    /// Images smaller than the region. BoofCV's meanB can't process these, so native is compared to the mean computed
    /// directly from its definition: a horizontal mean through the border, then a vertical mean through the border.
    /// Every pixel is a border pixel here, where BoofCV adds up each pixel times 1/length for floats, and adds up the
    /// pixels and rounds the division for integers.
    @Test
    public void meanBorder_smallerThanRegion() {
        Class[] types = new Class[]{GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class};
        int[][] shapes = new int[][]{{1, 1}, {3, 2}, {2, 5}};

        NativeImageBlurOps nativeBlur = new NativeImageBlurOps();
        for (Class type : types) {
            for (int[] shape : shapes) {
                for (ImageBorder border : supportedBorders(type)) {
                    ImageGray input = GeneralizedImageOps.createSingleBand(type, shape[0], shape[1]);
                    GImageMiscOps.fillUniform(input, rand, 0, 255);
                    input = BoofTesting.createSubImageOf(input);
                    ImageGray found = GeneralizedImageOps.createSingleBand(type, 1, 1);
                    ImageGray storage = GeneralizedImageOps.createSingleBand(type, 1, 1);
                    nativeBlur.processMeanBorder(input, found, 3, 3, border, storage);

                    ImageGray horizontal = naiveMeanBorder(input, 3, 0, border);
                    assertIdentical(naiveMeanBorder(horizontal, 0, 3, border), found);
                }
            }
        }
    }

    /// Native throws a RuntimeException for what it doesn't support, and BoofCV's meanB then uses its own code
    @Test
    public void meanBorder_unsupportedFallsBackToJava() {
        GrayU8 input = new GrayU8(20, 15);
        GImageMiscOps.fillUniform(input, rand, 0, 255);
        GrayU8 storage = new GrayU8(1, 1);

        List<ImageBorder> unsupported = new ArrayList<>();
        unsupported.add(FactoryImageBorder.singleValue(300, GrayU8.class));
        unsupported.add(FactoryImageBorder.singleValue(-1, GrayU8.class));
        unsupported.add(new ImageBorder1D_S32(new BorderIndex1D_Extend(), new BorderIndex1D_Wrap()));

        NativeImageBlurOps nativeBlur = new NativeImageBlurOps();
        for (ImageBorder border : unsupported) {
            assertThrows(RuntimeException.class,
                    () -> nativeBlur.processMeanBorder(input, new GrayU8(1, 1), 2, 2, border, storage));

            ImageGray expected = meanBorderJava(input, 2, 2, border, false);
            ImageGray found = meanBorderHook(input, 2, 2, border, false);
            assertIdentical(expected, found);
        }

        assertThrows(RuntimeException.class,
                () -> nativeBlur.processMeanBorder(input, new GrayU8(1, 1), 2, 2, null, storage));

        // meanB doesn't exist for GrayS16, but the hook's interface allows it
        GrayS16 inputS16 = new GrayS16(20, 15);
        ImageBorder borderS16 = FactoryImageBorder.single(BorderType.EXTENDED, GrayS16.class);
        assertThrows(RuntimeException.class,
                () -> nativeBlur.processMeanBorder(inputS16, new GrayS16(1, 1), 2, 2, borderS16, new GrayS16(1, 1)));
    }

    @Test
    public void meanBorder_badRadius() {
        GrayU8 input = new GrayU8(20, 15);
        ImageBorder border = FactoryImageBorder.single(BorderType.REFLECT, GrayU8.class);
        NativeImageBlurOps nativeBlur = new NativeImageBlurOps();
        assertThrows(IllegalArgumentException.class,
                () -> nativeBlur.processMeanBorder(input, new GrayU8(1, 1), 0, 2, border, null));
        assertThrows(IllegalArgumentException.class,
                () -> nativeBlur.processMeanBorder(input, new GrayU8(1, 1), 2, 0, border, null));
    }

    /// Borders native supports: extended, reflected, wrapped, zero and a constant value
    static List<ImageBorder> supportedBorders( Class type ) {
        List<ImageBorder> list = new ArrayList<>();
        for (BorderType b : new BorderType[]{BorderType.EXTENDED, BorderType.REFLECT, BorderType.WRAP, BorderType.ZERO})
            list.add(FactoryImageBorder.single(b, type));
        list.add(FactoryImageBorder.singleValue(type == GrayU8.class || type == GrayU16.class ? 7 : 7.5, type));
        return list;
    }

    void compareMeanBorder( ImageGray input, int radiusX, int radiusY, ImageBorder border, boolean concurrent ) {
        ImageGray expected = meanBorderJava(input, radiusX, radiusY, border, concurrent);

        NativeImageBlurOps nativeBlur = new NativeImageBlurOps();
        ImageGray found = (ImageGray)input.createSameShape();
        nativeBlur.processMeanBorder(input, found, radiusX, radiusY, border, null);
        assertIdentical(expected, found);

        assertIdentical(expected, meanBorderHook(input, radiusX, radiusY, border, concurrent));
    }

    /// BoofCV's meanB without the native hook
    static ImageGray meanBorderJava( ImageGray input, int radiusX, int radiusY, ImageBorder border,
                                     boolean concurrent ) {
        return callBoofCV(false, concurrent,
                () -> (ImageGray)GBlurImageOps.meanB(input, null, radiusX, radiusY, border, null, null));
    }

    /// BoofCV's meanB with BoofCPP's hook
    static ImageGray meanBorderHook( ImageGray input, int radiusX, int radiusY, ImageBorder border,
                                     boolean concurrent ) {
        return callBoofCV(true, concurrent,
                () -> (ImageGray)GBlurImageOps.meanB(input, null, radiusX, radiusY, border, null, null));
    }

    /// Mean along x over 2*radiusX+1 pixels, then along y over 2*radiusY+1 pixels, of every pixel read through the
    /// border. Same arithmetic as BoofCV's border code for a mean kernel.
    static ImageGray naiveMeanBorder( ImageGray input, int radiusX, int radiusY, ImageBorder border ) {
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

    static double get( ImageBorder border, int x, int y ) {
        double[] pixel = new double[1];
        border.getGeneral(x, y, pixel);
        return pixel[0];
    }
}
