package org.boofcpp.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeBlockMinMaxChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;

/// The block min-max threshold checks shared with the other variants, single threaded
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeBlockMinMax extends NativeBlockMinMaxChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeBlockMinMax() {
        super(NativeBlockMinMax::new, GrayU8.class, GrayF32.class);
    }

    /// Runs the _MT code with more than one thread
    @Test void concurrentToggle() {
        var filter = new NativeBlockMinMax<>(ConfigLength.fixed(5), 0.95, true, 1.0, true, GrayU8.class);
        var input = (GrayU8)randomSubImage(GrayU8.class, 40, 30, 0, 200);
        assertConcurrentToggle(true, () -> filter.process(input, new GrayU8(40, 30)));
    }
}
