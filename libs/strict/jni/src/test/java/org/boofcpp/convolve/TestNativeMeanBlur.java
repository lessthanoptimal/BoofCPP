package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.border.BorderType;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.NativeMeanBlurChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.boofcpp.testing.BoofCPPTesting.assertConcurrentToggle;
import static org.junit.jupiter.api.Assertions.assertThrows;

/// The mean blur checks shared with the other variants, without and with a border, single threaded. Strict's results
/// must be identical to BoofCV's single threaded code
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeMeanBlur extends NativeMeanBlurChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeMeanBlur() {
        super(new NativeImageBlurOps(), GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class);
    }

    /// Images the mean isn't native for throw, so BoofCV uses its Java code
    @Test void meanTypes() {
        var hook = new NativeImageBlurOps(GrayU16.class);
        for (Class type : new Class[]{GrayU8.class, GrayF32.class, GrayF64.class}) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 20, 15);
            for (BorderType border : new BorderType[]{BorderType.NORMALIZED, BorderType.REFLECT}) {
                assertThrows(RuntimeException.class, () ->
                        hook.processMean(input, input.createSameShape(), 2, 2, border, null));
            }
        }
        var input = new GrayU16(20, 15);
        hook.processMean(input, input.createSameShape(), 2, 2, BorderType.NORMALIZED, null);
    }

    /// Runs the _MT code with more than one thread. Normalized and with a border have separate switches
    @Test void concurrentToggle() {
        var hook = new NativeImageBlurOps();
        ImageGray input = randomSubImage(GrayU8.class, 40, 30, 0, 100);
        ImageGray output = (ImageGray)input.createSameShape();
        assertConcurrentToggle(true, () -> hook.processMean(input, output, 2, 2, BorderType.NORMALIZED, null));
        assertConcurrentToggle(true, () -> hook.processMean(input, output, 2, 2, BorderType.REFLECT, null));
    }

    /// The native functions reject a negative radius themselves, since C++ takes unsigned radii
    @Test void nativeNegativeRadius() {
        var hook = new NativeImageBlurOps();
        GrayU8 input = new GrayU8(10, 8), output = new GrayU8(10, 8), storage = new GrayU8(10, 8);
        assertThrows(IllegalArgumentException.class, () -> hook.nativeMeanU8(input, output, -1, 2, storage));
        assertThrows(IllegalArgumentException.class, () -> hook.nativeMeanU8(input, output, 2, -1, storage));
        assertThrows(IllegalArgumentException.class, () ->
                hook.nativeMeanBorderU8(input, output, -1, 2, BorderType.REFLECT, 0, storage));
        assertThrows(IllegalArgumentException.class, () ->
                hook.nativeMeanBorderU8(input, output, 2, -1, BorderType.REFLECT, 0, storage));
    }
}
