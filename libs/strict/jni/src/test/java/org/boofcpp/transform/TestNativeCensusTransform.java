package org.boofcpp.transform;

import boofcv.alg.transform.census.CensusTransform;
import boofcv.struct.border.BorderType;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import georegression.struct.point.Point2D_I32;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.testing.NativeCensusChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.ddogleg.struct.FastAccess;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;

/// The census transform checks shared with the other variants, single threaded
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeCensusTransform extends NativeCensusChecks {
    static {
        BoofCPPStrict.loadlib();
    }

    public TestNativeCensusTransform() {
        super(new NativeCensusTransform<>(), GrayU8.class, GrayU16.class, GrayF32.class);
    }

    /// Each switch runs the _MT code with more than one thread. Not dense5x5(), which shares dense3x3()'s switch
    @Test void concurrentToggle() {
        var hook = new NativeCensusTransform();
        FastAccess<Point2D_I32> samples = CensusTransform.createCircleSamples();
        ImageGray input = randomSubImage(GrayU8.class, 40, 30, 0, 100);
        assertConcurrentToggle(true, () -> hook.dense3x3(input, new GrayU8(40, 30), BorderType.REFLECT));
        assertConcurrentToggle(true, () -> hook.sample_S64(input, samples, new GrayS64(40, 30), BorderType.REFLECT));
    }
}
