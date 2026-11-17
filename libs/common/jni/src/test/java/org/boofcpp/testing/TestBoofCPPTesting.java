package org.boofcpp.testing;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.junit.jupiter.api.Test;
import org.opentest4j.AssertionFailedError;

import static org.junit.jupiter.api.Assertions.assertThrows;

public class TestBoofCPPTesting extends BoofCPPStandardJUnit {
    @Test
    public void assertMatches_scale() {
        // Absolute, unless scaled
        BoofCPPTesting.assertMatches(f64(1000.0), f64(1000.0 + 0.9e-12), Tolerance.FLOAT);
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertMatches(f64(1000.0), f64(1000.0 + 1.1e-12), Tolerance.FLOAT));
        BoofCPPTesting.assertMatches(f64(1000.0), f64(1000.0 + 0.9e-9), Tolerance.FLOAT, 1000.0);
        assertThrows(AssertionFailedError.class,
                () -> BoofCPPTesting.assertMatches(f64(1000.0), f64(1000.0 + 1.1e-9), Tolerance.FLOAT, 1000.0));
        // F32 uses its own tolerance
        BoofCPPTesting.assertMatches(f32(1.0f), f32(1.0f + 0.9e-5f), Tolerance.FLOAT);
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertMatches(f32(1.0f), f32(1.0f + 1.1e-5f), Tolerance.FLOAT));
    }

    @Test
    public void assertMatches_identical() {
        // Zero tolerance compares bits, so NaN matches NaN
        BoofCPPTesting.assertMatches(f32(Float.NaN), f32(Float.NaN), Tolerance.IDENTICAL);
        assertThrows(AssertionFailedError.class,
                () -> BoofCPPTesting.assertMatches(f32(1.0f), f32(Math.nextUp(1.0f)), Tolerance.IDENTICAL));
        // Integers are always identical
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertMatches(u8(10), u8(11), new Tolerance(1.0, 1.0)));
        // Shapes must match
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertMatches(new GrayF64(2, 1), new GrayF64(1, 1), Tolerance.FLOAT));
    }

    private static ImageGray<?> f32( float value ) {
        GrayF32 image = new GrayF32(1, 1);
        image.set(0, 0, value);
        return image;
    }

    private static ImageGray<?> f64( double value ) {
        GrayF64 image = new GrayF64(1, 1);
        image.set(0, 0, value);
        return image;
    }

    private static ImageGray<?> u8( int value ) {
        GrayU8 image = new GrayU8(1, 1);
        image.set(0, 0, value);
        return image;
    }

    /// The base class compares with its tolerance, which is identical results unless a test class changes it
    @Test
    public void standardJUnit_tolerance() {
        BoofCPPStandardJUnit test = new BoofCPPStandardJUnit();
        assertThrows(AssertionFailedError.class, () -> test.assertMatches(f32(1.0f), f32(Math.nextUp(1.0f))));
        test.tolerance = Tolerance.FLOAT;
        test.assertMatches(f32(1.0f), f32(Math.nextUp(1.0f)));
        assertThrows(AssertionFailedError.class, () -> test.assertMatches(f32(100.0f), f32(100.001f)));
        test.assertMatches(f32(100.0f), f32(100.001f), 100.0);
    }
}
