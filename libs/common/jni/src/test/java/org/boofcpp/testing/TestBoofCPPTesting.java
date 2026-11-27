package org.boofcpp.testing;

import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.NativeConcurrency;
import org.junit.jupiter.api.Test;
import org.opentest4j.AssertionFailedError;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class TestBoofCPPTesting extends BoofCPPStandardJUnit {
    @Test public void assertMatches_scale() {
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

    @Test public void assertMatches_identical() {
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
    @Test public void standardJUnit_tolerance() {
        BoofCPPStandardJUnit test = new BoofCPPStandardJUnit();
        assertThrows(AssertionFailedError.class, () -> test.assertMatches(f32(1.0f), f32(Math.nextUp(1.0f))));
        test.tolerance = Tolerance.FLOAT;
        test.assertMatches(f32(1.0f), f32(Math.nextUp(1.0f)));
        assertThrows(AssertionFailedError.class, () -> test.assertMatches(f32(100.0f), f32(100.001f)));
        test.assertMatches(f32(100.0f), f32(100.001f), 100.0);
    }

    /// Floating point disparity: scores where both are valid, or the disparities without scores, and agreement
    /// on which pixels are valid
    @Test public void assertDisparityScoresMatch() {
        // 3 pixels, invalid value 5. Pixel 1 picks another disparity with the same score, pixel 2 is invalid in both
        GrayU8 disparity = u8row(1, 2, 5);
        GrayF32 score = f32row(1000f, 0.5f, 7f);
        BoofCPPTesting.assertDisparityScoresMatch(disparity, score, u8row(1, 3, 5),
                f32row(1000f + 0.9e-2f, 0.5f + 0.9e-5f, 99f), 5, Tolerance.FLOAT, 1.0, 1.0);
        // Scores are relative to their magnitude, but at least 1
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertDisparityScoresMatch(disparity, score,
                disparity, f32row(1000f + 1.1e-2f, 0.5f, 7f), 5, Tolerance.FLOAT, 1.0, 1.0));
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertDisparityScoresMatch(disparity, score,
                disparity, f32row(1000f, 0.5f + 1.1e-5f, 7f), 5, Tolerance.FLOAT, 1.0, 1.0));
        // Without scores, the disparities. Here one of three is different
        BoofCPPTesting.assertDisparityScoresMatch(disparity, null, u8row(1, 4, 6), null, 5, Tolerance.FLOAT, 1.0, 0.66);
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertDisparityScoresMatch(disparity, null,
                u8row(1, 4, 6), null, 5, Tolerance.FLOAT, 1.0, 0.67));
        // Subpixel disparities are rounded
        BoofCPPTesting.assertDisparityScoresMatch(f32row(1.2f, 2.4f, 5f), null, f32row(0.8f, 1.6f, 5f), null, 5,
                Tolerance.FLOAT, 1.0, 1.0);
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertDisparityScoresMatch(f32row(1.2f, 2.4f,
                5f), null, f32row(1.2f, 2.6f, 5f), null, 5, Tolerance.FLOAT, 1.0, 1.0));
        // With scores, a different disparity is fine
        BoofCPPTesting.assertDisparityScoresMatch(disparity, score, u8row(0, 4, 6), score, 5, Tolerance.FLOAT,
                1.0, 1.0);
        // One of three pixels doesn't agree on being valid
        GrayU8 oneInvalid = u8row(1, 5, 5);
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertDisparityScoresMatch(disparity, score,
                oneInvalid, score, 5, Tolerance.FLOAT, 1.0, 1.0));
        BoofCPPTesting.assertDisparityScoresMatch(disparity, score, oneInvalid, score, 5, Tolerance.FLOAT, 0.66, 1.0);
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertDisparityScoresMatch(disparity, score,
                oneInvalid, score, 5, Tolerance.FLOAT, 0.67, 1.0));
        // Shapes must match
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertDisparityScoresMatch(disparity, score,
                new GrayU8(2, 1), score, 5, Tolerance.FLOAT, 1.0, 1.0));
    }

    /// Concurrency is off during the call and restored afterwards, even if it throws
    @Test public void singleThreaded() {
        boolean previous = BoofConcurrency.USE_CONCURRENT;
        try {
            BoofConcurrency.USE_CONCURRENT = true;
            assertEquals(5, BoofCPPTesting.singleThreaded(() -> {
                assertFalse(BoofConcurrency.USE_CONCURRENT);
                return 5;
            }));
            assertTrue(BoofConcurrency.USE_CONCURRENT);
            assertThrows(IllegalStateException.class, () -> BoofCPPTesting.singleThreaded((Runnable)() -> {
                throw new IllegalStateException();
            }));
            assertTrue(BoofConcurrency.USE_CONCURRENT);

            // An off setting stays off
            BoofConcurrency.USE_CONCURRENT = false;
            BoofCPPTesting.singleThreaded(() -> assertFalse(BoofConcurrency.USE_CONCURRENT));
            assertFalse(BoofConcurrency.USE_CONCURRENT);
        } finally {
            BoofConcurrency.USE_CONCURRENT = previous;
        }
    }

    /// Fails if the switch isn't what's expected, and restores the threads and testing blocks
    @Test public void assertConcurrentToggle() {
        BoofCPPTesting.assertConcurrentToggle(true, NativeToggleCall::call);
        BoofCPPTesting.assertConcurrentToggle(false, () -> {});
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertConcurrentToggle(false, NativeToggleCall::call));
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertConcurrentToggle(true, () -> {}));

        int threads = NativeConcurrency.getMaxThreads();
        int blocks = NativeConcurrency.getTestingBlocks();
        assertThrows(AssertionFailedError.class, () -> BoofCPPTesting.assertConcurrentToggle(true, () -> {}));
        assertEquals(threads, NativeConcurrency.getMaxThreads());
        assertEquals(blocks, NativeConcurrency.getTestingBlocks());
    }

    /// Inside the image the pixel's value, outside it the border's
    @Test public void borderValue() {
        GrayU8 u8 = u8row(3, 9);
        ImageBorder<GrayU8> extended = FactoryImageBorder.single(BorderType.EXTENDED, GrayU8.class);
        extended.setImage(u8);
        assertEquals(9, BoofCPPTesting.borderValue(extended, 1, 0));
        assertEquals(3, BoofCPPTesting.borderValue(extended, -2, 0));
        assertEquals(9, BoofCPPTesting.borderValue(extended, 4, 3));

        GrayF32 f32 = (GrayF32)f32(1.25f);
        ImageBorder<GrayF32> value = FactoryImageBorder.singleValue(7.5, GrayF32.class);
        value.setImage(f32);
        assertEquals(1.25, BoofCPPTesting.borderValue(value, 0, 0));
        assertEquals(7.5, BoofCPPTesting.borderValue(value, 0, -1));
    }

    private static GrayU8 u8row( int... values ) {
        GrayU8 image = new GrayU8(values.length, 1);
        for (int i = 0; i < values.length; i++)
            image.set(i, 0, values[i]);
        return image;
    }

    private static GrayF32 f32row( float... values ) {
        GrayF32 image = new GrayF32(values.length, 1);
        for (int i = 0; i < values.length; i++)
            image.set(i, 0, values[i]);
        return image;
    }
}
