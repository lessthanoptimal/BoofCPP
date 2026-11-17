package org.boofcpp.misc;

import boofcv.BoofTesting;
import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS8;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.Planar;
import org.boofcpp.BoofCPP;
import org.junit.jupiter.api.Test;

import java.util.Random;

import static org.boofcpp.convolve.TestNativeImageBlurOps.assertIdentical;
import static org.junit.jupiter.api.Assertions.assertThrows;

public class TestNativeAverageDownSampleOps {
    Random rand = new Random(23423);

    static {
        BoofCPP.loadlib();
    }

    /// Image types BoofCV's AverageDownSampleOps.down(input, sampleWidth, output) supports
    static final Class[] GRAY_TYPES = new Class[]{GrayU8.class, GrayS8.class, GrayU16.class, GrayS16.class,
            GrayS32.class, GrayF32.class, GrayF64.class};

    /// Against BoofCV's Java code, called directly and through BoofCV's hook. Width 2 and other widths are different
    /// implementations in BoofCV. Includes regions larger than the image
    @Test
    public void compareDown() {
        for (Class type : GRAY_TYPES) {
            for (int[] shape : new int[][]{{61, 45}, {6, 7}}) {
                for (boolean concurrent : new boolean[]{false, true}) {
                    for (int width : new int[]{1, 2, 3, 4, 5, 8, 70}) {
                        ImageGray input = randomImage(type, shape[0], shape[1]);
                        ImageGray expected = createOutput(input, width);
                        callBoofCV(false, concurrent, () -> AverageDownSampleOps.down(input, width, expected));

                        ImageGray found = BoofTesting.createSubImageOf(createOutput(input, width));
                        new NativeAverageDownSampleOps().processDown(input, width, found);
                        assertIdentical(expected, found);

                        ImageGray hook = createOutput(input, width);
                        callBoofCV(true, concurrent, () -> AverageDownSampleOps.down(input, width, hook));
                        assertIdentical(expected, hook);
                    }
                }
            }
        }
    }

    /// BoofCV down samples each band of a planar image with the gray image functions, so the hook is used for each one
    @Test
    public void down_planar() {
        Planar<GrayU8> input = new Planar<>(GrayU8.class, 31, 20, 3);
        GImageMiscOps.fillUniform(input, rand, 0, 255);
        Planar<GrayU8> expected = new Planar<>(GrayU8.class, 11, 7, 3);
        Planar<GrayU8> found = expected.createSameShape();

        callBoofCV(false, false, () -> AverageDownSampleOps.down(input, 3, expected));
        callBoofCV(true, false, () -> AverageDownSampleOps.down(input, 3, found));
        for (int band = 0; band < 3; band++) {
            assertIdentical(expected.getBand(band), found.getBand(band));
        }
    }

    /// Native throws a RuntimeException for anything it can't process, and BoofCV then uses its own code
    @Test
    public void down_unsupported() {
        NativeAverageDownSampleOps nativeOps = new NativeAverageDownSampleOps();
        GrayU8 input = new GrayU8(20, 15);

        assertThrows(IllegalArgumentException.class, () -> nativeOps.processDown(input, 0, new GrayU8(1, 1)));
        // the output must already be the down sampled size
        assertThrows(RuntimeException.class, () -> nativeOps.processDown(input, 2, new GrayU8(9, 8)));
        // the output must have the same pixel type
        assertThrows(RuntimeException.class, () -> nativeOps.processDown(input, 2, new GrayU16(10, 8)));
    }

    /// Image types BoofCV's AverageDownSampleOps.down(input, centered, output, middle) supports
    static final Class[] GENERAL_TYPES = new Class[]{GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class};

    /// Output sizes for a 61x45 input: integer and non-integer scales, the same size, and one pixel tall
    static final int[][] GENERAL_SHAPES = new int[][]{{61, 45}, {30, 15}, {20, 22}, {7, 5}, {13, 44}, {2, 1}};

    /// Against BoofCV's Java code, called directly and through BoofCV's hook
    @Test
    public void compareGeneral() {
        for (Class type : GENERAL_TYPES) {
            for (int[] shape : GENERAL_SHAPES) {
                for (boolean centered : new boolean[]{false, true}) {
                    for (boolean concurrent : new boolean[]{false, true}) {
                        ImageGray input = randomImage(type, 61, 45);
                        ImageGray expected = GeneralizedImageOps.createSingleBand(type, shape[0], shape[1]);
                        callBoofCV(false, concurrent, () -> AverageDownSampleOps.down(input, centered, expected, null));

                        ImageGray found = BoofTesting.createSubImageOf((ImageGray)expected.createSameShape());
                        new NativeAverageDownSampleOps().processDown(input, centered, found, null);
                        assertIdentical(expected, found);

                        ImageGray hook = (ImageGray)expected.createSameShape();
                        callBoofCV(true, concurrent, () -> AverageDownSampleOps.down(input, centered, hook, null));
                        assertIdentical(expected, hook);
                    }
                }
            }
        }
    }

    /// horizontal() and vertical() against BoofCV's Java code, called directly and through BoofCV's hooks. The input
    /// to vertical() is floating point for every output type
    @Test
    public void compareHorizontalVertical() {
        for (Class type : GENERAL_TYPES) {
            Class middleType = type == GrayF64.class ? GrayF64.class : GrayF32.class;
            for (int[] shape : GENERAL_SHAPES) {
                for (boolean centered : new boolean[]{false, true}) {
                    for (boolean concurrent : new boolean[]{false, true}) {
                        ImageGray input = randomImage(type, 61, 45);
                        ImageGray expected = GeneralizedImageOps.createSingleBand(middleType, shape[0], 45);
                        callBoofCV(false, concurrent, () -> AverageDownSampleOps.horizontal(input, centered, expected));
                        ImageGray found = (ImageGray)expected.createSameShape();
                        new NativeAverageDownSampleOps().processHorizontal(input, centered, found);
                        assertIdentical(expected, found);
                        callBoofCV(true, concurrent, () -> AverageDownSampleOps.horizontal(input, centered, found));
                        assertIdentical(expected, found);

                        ImageGray middle = randomImage(middleType, 61, 45);
                        ImageGray expectedV = GeneralizedImageOps.createSingleBand(type, 61, shape[1]);
                        callBoofCV(false, concurrent,
                                () -> AverageDownSampleOps.vertical(middle, centered, expectedV, null));
                        ImageGray foundV = (ImageGray)expectedV.createSameShape();
                        new NativeAverageDownSampleOps().processVertical(middle, centered, foundV);
                        assertIdentical(expectedV, foundV);
                        callBoofCV(true, concurrent,
                                () -> AverageDownSampleOps.vertical(middle, centered, foundV, null));
                        assertIdentical(expectedV, foundV);
                    }
                }
            }
        }
    }

    /// BoofCV down samples each band of a planar image with the gray image function, so the hook is used for each one
    @Test
    public void general_planar() {
        Planar<GrayU8> input = new Planar<>(GrayU8.class, 31, 20, 3);
        GImageMiscOps.fillUniform(input, rand, 0, 255);
        Planar<GrayU8> expected = new Planar<>(GrayU8.class, 12, 7, 3);
        Planar<GrayU8> found = expected.createSameShape();

        callBoofCV(false, false, () -> AverageDownSampleOps.down(input, true, expected, null));
        callBoofCV(true, false, () -> AverageDownSampleOps.down(input, true, found, null));
        for (int band = 0; band < 3; band++) {
            assertIdentical(expected.getBand(band), found.getBand(band));
        }
    }

    /// Native throws a RuntimeException for anything it can't process, and BoofCV then uses its own code
    @Test
    public void general_unsupported() {
        NativeAverageDownSampleOps nativeOps = new NativeAverageDownSampleOps();
        GrayU8 input = new GrayU8(20, 15);

        // BoofCV writes past the end of each row of 'middle' for a centered output one pixel wide
        assertThrows(RuntimeException.class, () -> nativeOps.processDown(input, true, new GrayU8(1, 5), null));
        // output larger than the input
        assertThrows(RuntimeException.class, () -> nativeOps.processDown(input, false, new GrayU8(21, 5), null));
        // middle must be the output's width and the input's height
        assertThrows(RuntimeException.class,
                () -> nativeOps.processDown(input, false, new GrayU8(10, 5), new GrayF32(10, 14)));
        // middle must be GrayF32 for a GrayU8 image
        assertThrows(RuntimeException.class,
                () -> nativeOps.processDown(input, false, new GrayU8(10, 5), new GrayF64(10, 15)));
        assertThrows(RuntimeException.class,
                () -> nativeOps.processDown(new Planar<>(GrayU8.class, 20, 15, 2), false,
                        new Planar<>(GrayU8.class, 10, 5, 2), null));
        assertThrows(RuntimeException.class, () -> nativeOps.processVertical(new GrayF32(20, 15), false,
                new GrayS16(20, 5)));
    }

    /// Random sub-image, so that its offset and stride aren't the defaults. Signed images have negative values
    ImageGray randomImage( Class type, int width, int height ) {
        ImageGray input = GeneralizedImageOps.createSingleBand(type, width, height);
        double min = 0, max = 255;
        if (type == GrayS8.class) {
            min = -128; max = 127;
        } else if (type == GrayU16.class) {
            max = 65535;
        } else if (type == GrayS16.class) {
            min = -32768; max = 32767;
        } else if (type == GrayS32.class) {
            min = -1_000_000; max = 1_000_000;
        } else if (type == GrayF32.class || type == GrayF64.class) {
            min = -100; max = 100;
        }
        GImageMiscOps.fillUniform(input, rand, min, max);
        return BoofTesting.createSubImageOf(input);
    }

    static ImageGray createOutput( ImageGray input, int width ) {
        return GeneralizedImageOps.createSingleBand(input.getClass(),
                AverageDownSampleOps.downSampleSize(input.width, width),
                AverageDownSampleOps.downSampleSize(input.height, width));
    }

    /// Calls BoofCV with BoofCPP's hooks, or without them
    static void callBoofCV( boolean useNative, boolean concurrent, Runnable call ) {
        BOverrideAverageDownSampleOps.Square previousSquare = BOverrideAverageDownSampleOps.square;
        BOverrideAverageDownSampleOps.General previousGeneral = BOverrideAverageDownSampleOps.general;
        BOverrideAverageDownSampleOps.Horizontal previousHorizontal = BOverrideAverageDownSampleOps.horizontal;
        BOverrideAverageDownSampleOps.Vertical previousVertical = BOverrideAverageDownSampleOps.vertical;
        boolean previousConcurrent = BoofConcurrency.USE_CONCURRENT;
        try {
            NativeAverageDownSampleOps nativeOps = useNative ? new NativeAverageDownSampleOps() : null;
            BOverrideAverageDownSampleOps.square = nativeOps;
            BOverrideAverageDownSampleOps.general = nativeOps;
            BOverrideAverageDownSampleOps.horizontal = nativeOps;
            BOverrideAverageDownSampleOps.vertical = nativeOps;
            BoofConcurrency.USE_CONCURRENT = concurrent;
            call.run();
        } finally {
            BOverrideAverageDownSampleOps.square = previousSquare;
            BOverrideAverageDownSampleOps.general = previousGeneral;
            BOverrideAverageDownSampleOps.horizontal = previousHorizontal;
            BOverrideAverageDownSampleOps.vertical = previousVertical;
            BoofConcurrency.USE_CONCURRENT = previousConcurrent;
        }
    }
}
