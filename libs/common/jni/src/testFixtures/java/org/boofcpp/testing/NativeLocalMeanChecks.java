package org.boofcpp.testing;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.ConfigLength;
import org.junit.jupiter.api.Test;

/// Checks of a variant's local mean threshold, shared by every variant. A variant's test class extends this and passes
/// its hook and the image types it supports
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeLocalMeanChecks extends ThresholdChecks {
    private final BOverrideFactoryThresholdBinary.LocalMean hook;

    protected NativeLocalMeanChecks( BOverrideFactoryThresholdBinary.LocalMean hook, Class... imageTypes ) {
        super(() -> BOverrideFactoryThresholdBinary.localMean = hook, imageTypes);
        this.hook = hook;
    }

    @Override protected InputToBinary create( Class type ) {
        return hook.handle(ConfigLength.fixed(5), 1.0, true, type);
    }

    @Test
    public void compareToBoofCV() {
        forEachInput(input -> {
            for (ConfigLength width : NativeBlockMeanChecks.WIDTHS) {
                for (boolean down : new boolean[]{true, false}) {
                    compare(input, () -> hook.handle(width, 0.95, down, input.getClass()),
                            () -> FactoryThresholdBinary.localMean(width, 0.95, down, input.getClass()));
                }
            }
        });
    }
}
