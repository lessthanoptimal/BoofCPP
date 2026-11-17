package org.boofcpp.testing;

import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.ImageGray;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;

/// Helpers shared by the tests of every variant
public class BoofCPPTesting {
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
