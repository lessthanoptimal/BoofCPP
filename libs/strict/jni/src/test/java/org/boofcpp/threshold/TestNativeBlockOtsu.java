package org.boofcpp.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.testing.NativeBlockOtsuChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;

/// The block otsu threshold checks shared with the other variants, single threaded
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeBlockOtsu extends NativeBlockOtsuChecks {
    static {
        BoofCPPStrict.loadlib();
    }

    public TestNativeBlockOtsu() {
        super(NativeBlockOtsu::new, GrayU8.class, GrayF32.class);
    }

    /// Runs the _MT code with more than one thread
    @Test void concurrentToggle() {
        var filter = new NativeBlockOtsu<>(false, ConfigLength.fixed(5), 0.3, 0.95, true, true, GrayU8.class);
        var input = (GrayU8)randomSubImage(GrayU8.class, 40, 30, 0, 200);
        assertConcurrentToggle(true, () -> filter.process(input, new GrayU8(40, 30)));
    }
}
