package org.boofcpp.accuracy;

import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.ImageGray;

/// How far an image is from the expected one
public class ImageDifference {
    /// Largest |found - expected|
    public double maxAbs;
    /// Largest |found - expected|/max(1, |expected|)
    public double maxRel;
    /// Root mean square of found - expected
    public double rms;
    /// Largest distance in representable values. For integer images it's the largest absolute difference
    public long maxUlp;
    /// Fraction of pixels which aren't identical
    public double differing;

    public static ImageDifference measure( ImageGray<?> expected, ImageGray<?> found ) {
        if (expected.width != found.width || expected.height != found.height)
            throw new IllegalArgumentException("Different shapes");

        ImageDifference d = new ImageDifference();
        long count = 0;
        double sumSq = 0;
        for (int y = 0; y < expected.height; y++) {
            for (int x = 0; x < expected.width; x++) {
                double e = GeneralizedImageOps.get(expected, x, y);
                double f = GeneralizedImageOps.get(found, x, y);
                long ulp;
                if (expected instanceof GrayF32) {
                    ulp = ulpDistance(((GrayF32)expected).get(x, y), ((GrayF32)found).get(x, y));
                } else if (expected instanceof GrayF64) {
                    ulp = ulpDistance(((GrayF64)expected).get(x, y), ((GrayF64)found).get(x, y));
                } else if (expected instanceof GrayS64) {
                    // A double can't hold every long, so differences are found by comparing the longs
                    long a = ((GrayS64)expected).get(x, y), b = ((GrayS64)found).get(x, y);
                    ulp = a == b ? 0 : Math.max(1, Math.abs(a - b));
                } else {
                    ulp = (long)Math.abs(f - e);
                }
                double error = Math.abs(f - e);
                d.maxAbs = Math.max(d.maxAbs, error);
                d.maxRel = Math.max(d.maxRel, error/Math.max(1.0, Math.abs(e)));
                d.maxUlp = Math.max(d.maxUlp, ulp);
                sumSq += error*error;
                if (ulp != 0)
                    count++;
            }
        }
        long total = (long)expected.width*expected.height;
        d.rms = total == 0 ? 0 : Math.sqrt(sumSq/total);
        d.differing = total == 0 ? 0 : count/(double)total;
        return d;
    }

    /// Number of representable floats between a and b
    public static long ulpDistance( float a, float b ) {
        return Math.abs(ordered(Float.floatToIntBits(a)) - ordered(Float.floatToIntBits(b)));
    }

    /// Number of representable doubles between a and b, saturated at Long.MAX_VALUE
    public static long ulpDistance( double a, double b ) {
        long oa = ordered(Double.doubleToLongBits(a)), ob = ordered(Double.doubleToLongBits(b));
        long diff = oa - ob;
        // the subtraction overflowed if the signs of the operands differ and the result's sign isn't oa's
        if (((oa ^ ob) & (oa ^ diff)) < 0)
            return Long.MAX_VALUE;
        return diff == Long.MIN_VALUE ? Long.MAX_VALUE : Math.abs(diff);
    }

    /// Maps the bits of a float to integers in the same order as the floats
    private static long ordered( int bits ) {
        return bits < 0 ? (long)Integer.MIN_VALUE - bits : bits;
    }

    private static long ordered( long bits ) {
        return bits < 0 ? Long.MIN_VALUE - bits : bits;
    }
}
