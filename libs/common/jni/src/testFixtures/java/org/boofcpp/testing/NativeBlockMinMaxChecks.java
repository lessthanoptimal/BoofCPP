package org.boofcpp.testing;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.ImageGray;
import org.junit.jupiter.api.Test;

/// Checks of a variant's block min-max threshold, shared by every variant. A variant's test class extends this and
/// passes its hook and the image types it supports
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeBlockMinMaxChecks extends ThresholdChecks {
    private final BOverrideFactoryThresholdBinary.LocalBlockMinMax hook;

    protected NativeBlockMinMaxChecks( BOverrideFactoryThresholdBinary.LocalBlockMinMax hook, Class... imageTypes ) {
        super(() -> BOverrideFactoryThresholdBinary.blockMinMax = hook, imageTypes);
        this.hook = hook;
    }

    @Override protected InputToBinary create( Class type ) {
        return hook.handle(ConfigLength.fixed(5), 1.0, true, 2.0, true, type);
    }

    @Test
    public void compareToBoofCV() {
        forEachInput(input -> {
            for (ConfigLength width : new ConfigLength[]{ConfigLength.fixed(2), ConfigLength.fixed(25)}) {
                for (boolean down : new boolean[]{true, false}) {
                    for (boolean local : new boolean[]{true, false}) {
                        compare(input, () -> hook.handle(width, 0.95, down, 2.0, local, input.getClass()),
                                () -> FactoryThresholdBinary.blockMinMax(width, 0.95, down, local, 2.0,
                                        input.getClass()));
                    }
                }
            }
        });
    }

    /// Floating point images with only negative values
    @Test
    public void negativePixels() {
        for (Class type : imageTypes) {
            if (!GeneralizedImageOps.isFloatingPoint(type))
                continue;
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 101, 77);
            GImageMiscOps.fillUniform(input, rand, -255, -1);
            for (boolean down : new boolean[]{true, false}) {
                compare(input, () -> hook.handle(ConfigLength.fixed(10), 1.0, down, 2.0, true, type),
                        () -> FactoryThresholdBinary.blockMinMax(ConfigLength.fixed(10), 1.0, down, true, 2.0, type));
            }
        }
    }

    /// Negative spreads on an image with flat and textured regions
    @Test
    public void unusualSpreads() {
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 101, 77);
            GImageMiscOps.fill(input, 128);
            GImageMiscOps.fillRectangle(input, 50, 0, 0, 40, 30);
            GImageMiscOps.fillUniform(input.subimage(60, 40, 101, 77), rand, 0, 255);
            for (double spread : new double[]{-1.0, -0.5, -0.1, 0.0}) {
                for (boolean down : new boolean[]{true, false}) {
                    compare(input, () -> hook.handle(ConfigLength.fixed(10), 1.0, down, spread, true, type),
                            () -> FactoryThresholdBinary.blockMinMax(ConfigLength.fixed(10), 1.0, down, true, spread,
                                    type));
                }
            }
        }
    }
}
