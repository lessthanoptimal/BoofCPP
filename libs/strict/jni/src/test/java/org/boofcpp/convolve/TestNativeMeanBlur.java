package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.NativeConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.NativeMeanBlurChecks;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.opentest4j.AssertionFailedError;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;
import static org.junit.jupiter.api.Assertions.assertThrows;

/// The mean blur checks shared with the other variants, single threaded. Strict's results must be identical to BoofCV's
/// single threaded code
public class TestNativeMeanBlur extends NativeMeanBlurChecks {
    static {
        BoofCPP.loadlib();
    }

    final int originalThreads = NativeConcurrency.getMaxThreads();

    public TestNativeMeanBlur() {
        super(new NativeImageBlurOps(), GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class);
    }

    @BeforeEach void singleThread() { NativeConcurrency.setMaxThreads(1); }

    @AfterEach void restoreThreads() { NativeConcurrency.setMaxThreads(originalThreads); }

    /// Integer images run the _MT code with more than one thread. Floating point ones are always single threaded
    @Test void concurrentToggle() {
        var hook = new NativeImageBlurOps();
        for (Class type : new Class[]{GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class}) {
            ImageGray input = randomSubImage(type, 40, 30, 0, 100);
            ImageGray output = (ImageGray)input.createSameShape();
            assertConcurrentToggle(!GeneralizedImageOps.isFloatingPoint(type),
                    () -> hook.processMeanWeighted(input, output, 2, 2, null));
        }
    }

    /// The check fails if the switch isn't what's expected
    @Test void concurrentToggle_fails() {
        var hook = new NativeImageBlurOps();
        ImageGray input = randomSubImage(GrayU8.class, 40, 30, 0, 100);
        ImageGray output = (ImageGray)input.createSameShape();
        assertThrows(AssertionFailedError.class, () ->
                assertConcurrentToggle(false, () -> hook.processMeanWeighted(input, output, 2, 2, null)));
        assertThrows(AssertionFailedError.class, () ->
                assertConcurrentToggle(true, () -> output.setTo(input)));
    }
}
