package org.boofcpp.fast.convolve;

import boofcv.struct.border.BorderType;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeMeanBlurChecks;
import org.boofcpp.testing.Tolerance;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertThrows;

/// The mean blur checks shared with the other variants, without and with a border, within floating point tolerance of
/// BoofCV. Fast supports GrayF32 and GrayF64
public class TestNativeMeanBlur extends NativeMeanBlurChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeMeanBlur() {
        super(new NativeImageBlurOps<>(null), GrayF32.class, GrayF64.class);
        tolerance = Tolerance.FLOAT;
    }

    /// The native functions reject a negative radius themselves, since C++ takes unsigned radii
    @Test void nativeNegativeRadius() {
        var hook = new NativeImageBlurOps<GrayF32>(null);
        GrayF32 input = new GrayF32(10, 8), output = new GrayF32(10, 8);
        assertThrows(IllegalArgumentException.class, () -> hook.nativeMeanF32(input, output, -1, 2));
        assertThrows(IllegalArgumentException.class, () -> hook.nativeMeanF32(input, output, 2, -1));
        assertThrows(IllegalArgumentException.class, () ->
                hook.nativeMeanBorderF32(input, output, -1, 2, BorderType.REFLECT, 0));
        assertThrows(IllegalArgumentException.class, () ->
                hook.nativeMeanBorderF32(input, output, 2, -1, BorderType.REFLECT, 0));
    }
}
