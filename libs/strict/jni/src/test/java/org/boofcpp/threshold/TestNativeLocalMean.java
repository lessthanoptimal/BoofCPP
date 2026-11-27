package org.boofcpp.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeLocalMeanChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;

/// The local mean threshold checks shared with the other variants, single threaded
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeLocalMean extends NativeLocalMeanChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeLocalMean() {
        super(NativeLocalMean::new, GrayU8.class, GrayF32.class);
    }

    /// Runs the _MT code with more than one thread. U8 and the other types have separate switches
    @Test void concurrentToggle() {
        for (Class type : new Class[]{GrayU8.class, GrayF32.class}) {
            var filter = new NativeLocalMean<>(ConfigLength.fixed(5), 0.95, true, type);
            ImageGray input = randomSubImage(type, 40, 30, 0, 200);
            assertConcurrentToggle(true, () -> filter.process(input, new GrayU8(40, 30)));
        }
    }
}
