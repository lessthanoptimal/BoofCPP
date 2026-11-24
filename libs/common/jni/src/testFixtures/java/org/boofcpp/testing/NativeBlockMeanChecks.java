package org.boofcpp.testing;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.ConfigLength;
import org.junit.jupiter.api.Test;

/// Checks of a variant's block mean threshold, shared by every variant. A variant's test class extends this and passes
/// its hook and the image types it supports
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeBlockMeanChecks extends ThresholdChecks {
    /// Region widths: large and small fixed widths, and one relative to the image size
    static final ConfigLength[] WIDTHS = {ConfigLength.fixed(25), ConfigLength.fixed(7), ConfigLength.relative(0.05, 5)};

    private final BOverrideFactoryThresholdBinary.LocalBlockMean hook;

    protected NativeBlockMeanChecks( BOverrideFactoryThresholdBinary.LocalBlockMean hook, Class... imageTypes ) {
        super(() -> BOverrideFactoryThresholdBinary.blockMean = hook, imageTypes);
        this.hook = hook;
    }

    @Override protected InputToBinary create( Class type ) {
        return hook.handle(ConfigLength.fixed(5), 1.0, true, true, type);
    }

    @Test public void compareToBoofCV() {
        forEachInput(input -> {
            for (ConfigLength width : WIDTHS) {
                for (boolean down : new boolean[]{true, false}) {
                    for (boolean local : new boolean[]{true, false}) {
                        compare(input, () -> hook.handle(width, 0.95, down, local, input.getClass()),
                                () -> FactoryThresholdBinary.blockMean(width, 0.95, down, local, input.getClass()));
                    }
                }
            }
        });
    }

    /// Scales whose double and float products round differently, and negative scales
    @Test public void unusualScales() {
        forEachInput(input -> {
            for (double scale : new double[]{0.82, -0.5}) {
                for (boolean down : new boolean[]{true, false}) {
                    compare(input, () -> hook.handle(ConfigLength.fixed(2), scale, down, true, input.getClass()),
                            () -> FactoryThresholdBinary.blockMean(ConfigLength.fixed(2), scale, down, true,
                                    input.getClass()));
                }
            }
        });
    }
}
