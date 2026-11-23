package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.alg.misc.GImageMiscOps;
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
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's average down sampling hooks against BoofCV's Java code, shared by every variant. A variant's
/// test class extends this and passes its hook and the image types it supports
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeAverageDownSampleChecks extends BoofCPPStandardJUnit {
    /// Image types BoofCV's AverageDownSampleOps.down(input, centered, output, middle) supports
    static final List<Class> BOOFCV_GENERAL_TYPES = Arrays.asList(GrayU8.class, GrayU16.class, GrayF32.class,
            GrayF64.class);

    private final BOverrideAverageDownSampleOps.Square square;
    private final BOverrideAverageDownSampleOps.General general;
    private final BOverrideAverageDownSampleOps.Horizontal horizontal;
    private final BOverrideAverageDownSampleOps.Vertical vertical;
    /// Types the variant supports, for down(input, sampleWidth, output)
    private final Class[] imageTypes;
    /// Types the variant supports which BoofCV's other down sampling functions support
    private final List<Class> generalTypes = new ArrayList<>();

    protected <H extends BOverrideAverageDownSampleOps.Square & BOverrideAverageDownSampleOps.General &
            BOverrideAverageDownSampleOps.Horizontal & BOverrideAverageDownSampleOps.Vertical>
    NativeAverageDownSampleChecks( H hook, Class... imageTypes ) {
        this.square = hook;
        this.general = hook;
        this.horizontal = hook;
        this.vertical = hook;
        this.imageTypes = imageTypes;
        for (Class type : imageTypes) {
            if (BOOFCV_GENERAL_TYPES.contains(type))
                generalTypes.add(type);
        }
    }

    /// Against BoofCV's Java code. Width 2 and other widths are different implementations in BoofCV. Includes regions
    /// larger than the image
    @Test
    public void compareDown() {
        for (Class type : imageTypes) {
            for (int[] shape : new int[][]{{61, 45}, {6, 7}}) {
                for (int width : new int[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 70}) {
                    ImageGray input = randomImage(type, shape[0], shape[1]);
                    ImageGray expected = createOutput(input, width);
                    callBoofCV(false, () -> AverageDownSampleOps.down(input, width, expected));

                    ImageGray found = BoofTesting.createSubImageOf(createOutput(input, width));
                    square.processDown(input, width, found);
                    assertMatches(expected, found);
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

        callBoofCV(false, () -> AverageDownSampleOps.down(input, 3, expected));
        callBoofCV(true, () -> AverageDownSampleOps.down(input, 3, found));
        for (int band = 0; band < 3; band++) {
            assertMatches(expected.getBand(band), found.getBand(band));
        }
    }

    /// The variant throws a RuntimeException for anything it can't process, and BoofCV then uses its own code
    @Test
    public void down_unsupported() {
                GrayU8 input = new GrayU8(20, 15);

        assertThrows(IllegalArgumentException.class, () -> square.processDown(input, 0, new GrayU8(1, 1)));
        // the output must already be the down sampled size
        assertThrows(RuntimeException.class, () -> square.processDown(input, 2, new GrayU8(9, 8)));
        // the output must have the same pixel type
        assertThrows(RuntimeException.class, () -> square.processDown(input, 2, new GrayU16(10, 8)));
    }

    /// Types the variant doesn't support throw a RuntimeException, so BoofCV uses its own code
    @Test
    public void unsupportedTypesThrow() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            assertThrows(RuntimeException.class, () -> square.processDown(input, 2, createOutput(input, 2)));
        }
        for (Class type : BoofCPPTesting.unsupported(generalTypes.toArray(new Class[0]))) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            ImageGray output = GeneralizedImageOps.createSingleBand(type, 10, 5);
            assertThrows(RuntimeException.class, () -> general.processDown(input, false, output, null));
            assertThrows(RuntimeException.class, () -> horizontal.processHorizontal(input, false,
                    new GrayF32(10, 15)));
        }
    }

    /// Output sizes for a 61x45 input: integer and non-integer scales, the same size, and one pixel tall
    static final int[][] GENERAL_SHAPES = new int[][]{{61, 45}, {30, 15}, {20, 22}, {7, 5}, {13, 44}, {2, 1}};

    /// Against BoofCV's Java code
    @Test
    public void compareGeneral() {
        for (Class type : generalTypes) {
            for (int[] shape : GENERAL_SHAPES) {
                for (boolean centered : new boolean[]{false, true}) {
                    ImageGray input = randomImage(type, 61, 45);
                    ImageGray expected = GeneralizedImageOps.createSingleBand(type, shape[0], shape[1]);
                    callBoofCV(false, () -> AverageDownSampleOps.down(input, centered, expected, null));

                    ImageGray found = BoofTesting.createSubImageOf((ImageGray)expected.createSameShape());
                    general.processDown(input, centered, found, null);
                    assertMatches(expected, found);
                }
            }
        }
    }

    /// horizontal() and vertical() against BoofCV's Java code. The input to vertical() is floating point for every
    /// output type
    @Test
    public void compareHorizontalVertical() {
        for (Class type : generalTypes) {
            Class middleType = type == GrayF64.class ? GrayF64.class : GrayF32.class;
            for (int[] shape : GENERAL_SHAPES) {
                for (boolean centered : new boolean[]{false, true}) {
                    ImageGray input = randomImage(type, 61, 45);
                    ImageGray expected = GeneralizedImageOps.createSingleBand(middleType, shape[0], 45);
                    callBoofCV(false, () -> AverageDownSampleOps.horizontal(input, centered, expected));
                    ImageGray found = (ImageGray)expected.createSameShape();
                    horizontal.processHorizontal(input, centered, found);
                    assertMatches(expected, found);

                    ImageGray middle = randomImage(middleType, 61, 45);
                    ImageGray expectedV = GeneralizedImageOps.createSingleBand(type, 61, shape[1]);
                    callBoofCV(false, () -> AverageDownSampleOps.vertical(middle, centered, expectedV, null));
                    ImageGray foundV = (ImageGray)expectedV.createSameShape();
                    vertical.processVertical(middle, centered, foundV);
                    assertMatches(expectedV, foundV);
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

        callBoofCV(false, () -> AverageDownSampleOps.down(input, true, expected, null));
        callBoofCV(true, () -> AverageDownSampleOps.down(input, true, found, null));
        for (int band = 0; band < 3; band++) {
            assertMatches(expected.getBand(band), found.getBand(band));
        }
    }

    /// The variant throws a RuntimeException for anything it can't process, and BoofCV then uses its own code
    @Test
    public void general_unsupported() {
                GrayU8 input = new GrayU8(20, 15);

        // BoofCV writes past the end of each row of 'middle' for a centered output one pixel wide
        assertThrows(RuntimeException.class, () -> general.processDown(input, true, new GrayU8(1, 5), null));
        // output larger than the input
        assertThrows(RuntimeException.class, () -> general.processDown(input, false, new GrayU8(21, 5), null));
        // middle must be the output's width and the input's height
        assertThrows(RuntimeException.class,
                () -> general.processDown(input, false, new GrayU8(10, 5), new GrayF32(10, 14)));
        // middle must be GrayF32 for a GrayU8 image
        assertThrows(RuntimeException.class,
                () -> general.processDown(input, false, new GrayU8(10, 5), new GrayF64(10, 15)));
        assertThrows(RuntimeException.class,
                () -> general.processDown(new Planar<>(GrayU8.class, 20, 15, 2), false,
                        new Planar<>(GrayU8.class, 10, 5, 2), null));
        assertThrows(RuntimeException.class, () -> vertical.processVertical(new GrayF32(20, 15), false,
                new GrayS16(20, 5)));
    }

    /// Random sub-image, so that its offset and stride aren't the defaults. Signed images have negative values
    ImageGray randomImage( Class type, int width, int height ) {
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
        return randomSubImage(type, width, height, min, max);
    }

    static ImageGray createOutput( ImageGray input, int width ) {
        return GeneralizedImageOps.createSingleBand(input.getClass(),
                AverageDownSampleOps.downSampleSize(input.width, width),
                AverageDownSampleOps.downSampleSize(input.height, width));
    }

    /// Calls BoofCV single threaded with the variant's hooks, or without them
    void callBoofCV( boolean useNative, Runnable call ) {
        BoofCPPTesting.singleThreaded(() -> {
            if (useNative)
                AverageDownSampleHooks.run(square, general, horizontal, vertical, call);
            else
                AverageDownSampleHooks.withoutHooks(call);
        });
    }
}
