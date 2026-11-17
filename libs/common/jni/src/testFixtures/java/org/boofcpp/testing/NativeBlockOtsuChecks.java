package org.boofcpp.testing;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.ConfigLength;
import org.junit.jupiter.api.Test;

/// Checks of a variant's block Otsu threshold, shared by every variant. A variant's test class extends this and passes
/// its hook and the image types it supports
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeBlockOtsuChecks extends ThresholdChecks {
    private final BOverrideFactoryThresholdBinary.LocalBlockOtsu hook;

    protected NativeBlockOtsuChecks( BOverrideFactoryThresholdBinary.LocalBlockOtsu hook, Class... imageTypes ) {
        super(() -> BOverrideFactoryThresholdBinary.blockOtsu = hook, imageTypes);
        this.hook = hook;
    }

    @Override protected InputToBinary create( Class type ) {
        return hook.handle(false, ConfigLength.fixed(5), 0.1, 1.0, true, true, type);
    }

    @Test
    public void compareToBoofCV() {
        forEachInput(input -> {
            for (ConfigLength width : NativeBlockMeanChecks.WIDTHS) {
                for (boolean down : new boolean[]{true, false}) {
                    for (boolean local : new boolean[]{true, false}) {
                        for (boolean otsu2 : new boolean[]{true, false}) {
                            compare(input, () -> hook.handle(otsu2, width, 0.05, 0.95, down, local, input.getClass()),
                                    () -> FactoryThresholdBinary.blockOtsu(width, 0.95, down, local, otsu2, 0.05,
                                            input.getClass()));
                        }
                    }
                }
            }
        });
    }
}
