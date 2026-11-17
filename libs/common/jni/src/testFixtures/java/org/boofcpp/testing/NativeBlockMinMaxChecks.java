package org.boofcpp.testing;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.ConfigLength;
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
}
