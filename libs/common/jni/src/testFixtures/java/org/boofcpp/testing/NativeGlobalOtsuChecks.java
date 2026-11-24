package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.abst.filter.binary.GlobalBinaryFilter;
import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's global Otsu threshold, shared by every variant. A variant's test class extends this and passes
/// its hook and the image types it supports
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeGlobalOtsuChecks extends ThresholdChecks {
    private final BOverrideFactoryThresholdBinary.GlobalOtsu hook;

    protected NativeGlobalOtsuChecks( BOverrideFactoryThresholdBinary.GlobalOtsu hook, Class... imageTypes ) {
        super(() -> BOverrideFactoryThresholdBinary.globalOtsu = hook, imageTypes);
        this.hook = hook;
    }

    @Override protected InputToBinary create( Class type ) {
        return hook.handle(0, 255, 1.0, true, type);
    }

    /// Pixel ranges which start at 0 and which shift the histogram, and scales that move the threshold below 0 and
    /// above 255
    @Test public void compareToBoofCV() {
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 100, 90);
            for (double[] range : new double[][]{{0, 255}, {20, 200}}) {
                GImageMiscOps.fillUniform(input, rand, range[0], range[1]);
                for (double scale : new double[]{1.0, 0.0, 0.5, 0.95, 1.15, 3.0}) {
                    for (boolean down : new boolean[]{true, false}) {
                        compare(input, () -> hook.handle(range[0], range[1], scale, down, type),
                                () -> FactoryThresholdBinary.globalOtsu(range[0], range[1], scale, down, type));
                    }
                }
            }
        }
    }

    /// Bounds outside the pixel type's range and fractional bounds. BoofCV truncates them for the histogram and adds
    /// minValue back to the threshold
    @Test public void boundsBeyondPixels() {
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 60, 50);
            GImageMiscOps.fillUniform(input, rand, 20, 200);
            for (double[] range : new double[][]{{0, 300}, {-10, 255}, {0.5, 255.7}, {-3.25, 270.5}, {19.9, 201.2}}) {
                for (boolean down : new boolean[]{true, false}) {
                    compare(input, () -> hook.handle(range[0], range[1], 1.0, down, type),
                            () -> FactoryThresholdBinary.globalOtsu(range[0], range[1], 1.0, down, type));
                }
            }
            // (int)(1 + 200.2 - 20.9) = 180 bins from 20, so a pixel of 200 has no bin
            if (type == GrayU8.class) {
                ((GrayU8)input).set(0, 0, 200);
                GrayU8 output = new GrayU8(input.width, input.height);
                assertThrows(RuntimeException.class, () -> new GlobalBinaryFilter.Otsu(20.9, 200.2, 1.0, true,
                        ImageType.single(type)).process(input, output));
                assertThrows(RuntimeException.class,
                        () -> hook.handle(20.9, 200.2, 1.0, true, type).process(input, output));
            }
        }
    }

    /// Pixels outside [minValue, maxValue] are handled as in BoofCV. F32 values less than 1 outside and NaN are
    /// counted, others throw
    @Test public void pixelsOutsideRange() {
        if (supports(GrayF32.class)) {
            GrayF32 input = new GrayF32(30, 20);
            GImageMiscOps.fillUniform(input, rand, 20, 200);
            input.set(0, 0, 19.5f);
            input.set(1, 0, 200.9f);
            input.set(2, 0, Float.NaN);
            GrayU8 expected = new GrayU8(30, 20);
            GrayU8 found = new GrayU8(30, 20);
            boofcv(GrayF32.class).process(input, expected);
            hook.handle(20, 200, 1.0, true, GrayF32.class).process(input, found);
            BoofTesting.assertEquals(expected, found, 0);

            for (float value : new float[]{19f, 201f, Float.POSITIVE_INFINITY}) {
                input.set(2, 0, value);
                checkBothThrow(input, GrayF32.class);
            }
        }

        if (supports(GrayU8.class)) {
            GrayU8 input = new GrayU8(30, 20);
            GImageMiscOps.fillUniform(input, rand, 20, 200);
            for (int value : new int[]{19, 201, 0, 255}) {
                input.set(2, 0, value);
                checkBothThrow(input, GrayU8.class);
            }
        }
    }

    private boolean supports( Class type ) {
        for (Class c : imageTypes) {
            if (c == type)
                return true;
        }
        return false;
    }

    /// BoofCV's own filter for pixels in [20, 200]
    private static InputToBinary boofcv( Class type ) {
        return new GlobalBinaryFilter.Otsu(20, 200, 1.0, true, ImageType.single(type));
    }

    private void checkBothThrow( ImageGray input, Class type ) {
        GrayU8 output = new GrayU8(input.width, input.height);
        assertThrows(RuntimeException.class, () -> boofcv(type).process(input, output));
        assertThrows(RuntimeException.class, () -> hook.handle(20, 200, 1.0, true, type).process(input, output));
    }
}
