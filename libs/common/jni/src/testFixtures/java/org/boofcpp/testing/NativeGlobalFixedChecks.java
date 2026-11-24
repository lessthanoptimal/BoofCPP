package org.boofcpp.testing;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import org.junit.jupiter.api.Test;

/// Checks of a variant's global fixed threshold, shared by every variant. A variant's test class extends this and passes
/// its hook and the image types it supports
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeGlobalFixedChecks extends ThresholdChecks {
    private final BOverrideFactoryThresholdBinary.GlobalFixed hook;

    protected NativeGlobalFixedChecks( BOverrideFactoryThresholdBinary.GlobalFixed hook, Class... imageTypes ) {
        super(() -> BOverrideFactoryThresholdBinary.globalFixed = hook, imageTypes);
        this.hook = hook;
    }

    @Override protected InputToBinary create( Class type ) {
        return hook.handle(10, true, type);
    }

    /// Thresholds which are fractional and outside the pixel range
    @Test public void compareToBoofCV() {
        forEachInput(input -> {
            for (double threshold : new double[]{125, 125.5, -1, 0, 255, 300}) {
                for (boolean down : new boolean[]{true, false}) {
                    compare(input, () -> hook.handle(threshold, down, input.getClass()),
                            () -> FactoryThresholdBinary.globalFixed(threshold, down, input.getClass()));
                }
            }
        });
    }
}
