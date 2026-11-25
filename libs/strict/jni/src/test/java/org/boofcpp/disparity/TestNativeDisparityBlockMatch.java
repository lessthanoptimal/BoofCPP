package org.boofcpp.disparity;

import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeDisparityBlockMatchChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;

/// The block matching checks shared with the other variants, single threaded
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeDisparityBlockMatch extends NativeDisparityBlockMatchChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeDisparityBlockMatch() {
        super(NativeDisparityBlockMatch::create, GrayU8.class, GrayF32.class);
    }

    /// Runs the _MT code with more than one thread
    @Test void concurrentToggle() {
        var config = new ConfigDisparityBM();
        config.errorType = DisparityError.SAD;
        config.disparityRange = 20;
        config.subpixel = false;

        var left = (GrayU8)randomSubImage(GrayU8.class, 40, 30, 0, 255);
        var right = (GrayU8)randomSubImage(GrayU8.class, 40, 30, 0, 255);
        var algU8 = NativeDisparityBlockMatch.create(config, GrayU8.class, GrayU8.class);
        assertConcurrentToggle(true, () -> algU8.process(left, right));
    }
}
