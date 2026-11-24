package org.boofcpp.testing;

import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.*;
import org.boofcpp.NativeConcurrency;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.function.Supplier;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

/// Helpers shared by the tests of every variant
@SuppressWarnings({"rawtypes"})
public class BoofCPPTesting {
    /// Every gray image type
    public static final Class[] GRAY_TYPES = {GrayU8.class, GrayS8.class, GrayU16.class, GrayS16.class,
            GrayS32.class, GrayS64.class, GrayF32.class, GrayF64.class};

    /// Gray image types in GRAY_TYPES that aren't in 'supported'
    public static List<Class> unsupported( Class... supported ) {
        List<Class> types = new ArrayList<>(Arrays.asList(GRAY_TYPES));
        types.removeAll(Arrays.asList(supported));
        return types;
    }

    /// [#assertMatches(ImageGray, ImageGray, Tolerance, double)] with a scale of 1
    public static void assertMatches( ImageGray<?> expected, ImageGray<?> found, Tolerance tolerance ) {
        assertMatches(expected, found, tolerance, 1.0);
    }

    /// Each pixel must satisfy |found - expected| <= tol*scale, with tol for the image's type. A tolerance of 0 and
    /// integer images require identical values. See CodeStandards.md, Testing Fast, before changing scale
    public static void assertMatches( ImageGray<?> expected, ImageGray<?> found, Tolerance tolerance, double scale ) {
        double tol = expected instanceof GrayF32 ? tolerance.f32 : expected instanceof GrayF64 ? tolerance.f64 : 0.0;
        if (tol == 0.0) {
            assertIdentical(expected, found);
            return;
        }
        assertEquals(expected.width, found.width);
        assertEquals(expected.height, found.height);
        for (int y = 0; y < expected.height; y++) {
            for (int x = 0; x < expected.width; x++) {
                double e = GeneralizedImageOps.get(expected, x, y);
                double f = GeneralizedImageOps.get(found, x, y);
                if (!(Math.abs(f - e) <= tol*scale))
                    fail("Values not within " + tol*scale + " at (" + x + "," + y + ") " + e + " " + f);
            }
        }
    }

    /// Disparity selected from floating point scores, e.g. of GrayF32 images or NCC. A near tie can select another
    /// disparity, or change whether a pixel is valid. At least 'minValid' of the pixels must agree on whether they are
    /// valid. With scores, where both are valid, the score of the selected disparity must satisfy
    /// |found - expected| <= tol*max(1, |expected|), since a score is a sum over the region. Without scores, at least
    /// 'minDisparity' of the pixels must have the same disparity, rounded to an integer, or both be invalid
    ///
    /// @param invalid Disparity values at least this are invalid
    /// @param scoreExpected Scores, or null if there are none
    public static void assertDisparityScoresMatch( ImageGray<?> disparityExpected, GrayF32 scoreExpected,
                                                   ImageGray<?> disparityFound, GrayF32 scoreFound, int invalid,
                                                   Tolerance tolerance, double minValid, double minDisparity ) {
        assertEquals(disparityExpected.width, disparityFound.width);
        assertEquals(disparityExpected.height, disparityFound.height);
        int disagreeValid = 0;
        int disagreeDisparity = 0;
        for (int y = 0; y < disparityExpected.height; y++) {
            for (int x = 0; x < disparityExpected.width; x++) {
                double dE = GeneralizedImageOps.get(disparityExpected, x, y);
                double dF = GeneralizedImageOps.get(disparityFound, x, y);
                boolean validExpected = dE < invalid;
                if (validExpected != dF < invalid) {
                    disagreeValid++;
                    disagreeDisparity++;
                    continue;
                }
                if (!validExpected)
                    continue;
                if (scoreExpected == null) {
                    if (Math.round(dE) != Math.round(dF))
                        disagreeDisparity++;
                    continue;
                }
                double e = scoreExpected.get(x, y);
                double f = scoreFound.get(x, y);
                double tol = tolerance.f32*Math.max(1.0, Math.abs(e));
                if (!(Math.abs(f - e) <= tol))
                    fail("Score not within " + tol + " at (" + x + "," + y + ") " + e + " " + f);
            }
        }
        int pixels = disparityExpected.width*disparityExpected.height;
        if (pixels - disagreeValid < minValid*pixels)
            fail(disagreeValid + " of " + pixels + " pixels don't agree on being valid");
        if (scoreExpected == null && pixels - disagreeDisparity < minDisparity*pixels)
            fail(disagreeDisparity + " of " + pixels + " pixels have a different disparity");
    }

    /// Runs 'call' with BoofCV's concurrency off, so its Java code is single threaded, as expected results are. The
    /// setting is restored afterwards
    public static <R> R singleThreaded( Supplier<R> call ) {
        boolean previous = BoofConcurrency.USE_CONCURRENT;
        try {
            BoofConcurrency.USE_CONCURRENT = false;
            return call.get();
        } finally {
            BoofConcurrency.USE_CONCURRENT = previous;
        }
    }

    /// [#singleThreaded(Supplier)] for a call without a result
    public static void singleThreaded( Runnable call ) {
        singleThreaded(() -> {
            call.run();
            return null;
        });
    }

    /// The switch between a native call's single threaded and concurrent code. With one thread 'call' must run
    /// single threaded code. With two it must run concurrent code if 'concurrent' is true and single threaded code if
    /// it's false. Results aren't checked. Two threads work on a single core too, since the switch only looks at the
    /// thread count. The settings are restored afterwards
    public static void assertConcurrentToggle( boolean concurrent, Runnable call ) {
        int threads = NativeConcurrency.getMaxThreads();
        int blocks = NativeConcurrency.getTestingBlocks();
        try {
            // See if there was a new concurrent loop called. Single thread variant will do nothing
            NativeConcurrency.setTestingBlocks(4);
            NativeConcurrency.setMaxThreads(1);
            long before = NativeConcurrency.getTestingLoops();
            call.run();
            assertEquals(0, NativeConcurrency.getTestingLoops() - before, "Concurrent code ran with one thread");

            NativeConcurrency.setMaxThreads(2);
            before = NativeConcurrency.getTestingLoops();
            call.run();
            long loops = NativeConcurrency.getTestingLoops() - before;
            if (concurrent)
                assertTrue(loops > 0, "Single threaded code ran with two threads");
            else
                assertEquals(0, loops, "Concurrent code ran for a single threaded call");
        } finally {
            NativeConcurrency.setMaxThreads(threads);
            NativeConcurrency.setTestingBlocks(blocks);
        }
    }

    /// Pixel values must be identical. Floats are compared by their bits, so NaN must match NaN
    public static void assertIdentical( ImageGray<?> expected, ImageGray<?> found ) {
        assertEquals(expected.width, found.width);
        assertEquals(expected.height, found.height);
        for (int y = 0; y < expected.height; y++) {
            for (int x = 0; x < expected.width; x++) {
                boolean same;
                if (expected instanceof GrayF32) {
                    same = Float.floatToIntBits(((GrayF32)expected).get(x, y)) ==
                            Float.floatToIntBits(((GrayF32)found).get(x, y));
                } else if (expected instanceof GrayF64) {
                    same = Double.doubleToLongBits(((GrayF64)expected).get(x, y)) ==
                            Double.doubleToLongBits(((GrayF64)found).get(x, y));
                } else {
                    same = GeneralizedImageOps.get(expected, x, y) == GeneralizedImageOps.get(found, x, y);
                }
                if (!same)
                    fail("Values not equal at (" + x + "," + y + ") " + GeneralizedImageOps.get(expected, x, y) +
                            " " + GeneralizedImageOps.get(found, x, y));
            }
        }
    }
}
