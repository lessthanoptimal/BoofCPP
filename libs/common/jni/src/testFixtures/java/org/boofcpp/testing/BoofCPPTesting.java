package org.boofcpp.testing;

import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
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
