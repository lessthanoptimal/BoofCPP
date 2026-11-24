package org.boofcpp.convolve;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.NativeMeanBlurChecks;
import org.boofcpp.testing.NativeSingleThreaded;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.opentest4j.AssertionFailedError;

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

    /// Same as concurrentToggle() with a border
    @Test void concurrentToggleWithBorder() {
        var hook = new NativeImageBlurOps();
        for (Class type : new Class[]{GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class}) {
            ImageGray input = randomSubImage(type, 40, 30, 0, 100);
            ImageGray output = (ImageGray)input.createSameShape();
            ImageBorder border = FactoryImageBorder.single(BorderType.REFLECT, type);
            assertConcurrentToggle(!GeneralizedImageOps.isFloatingPoint(type),
                    () -> hook.processMeanBorder(input, output, 2, 2, border, null));
        }
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
