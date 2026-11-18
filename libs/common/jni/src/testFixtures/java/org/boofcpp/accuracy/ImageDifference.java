package org.boofcpp.accuracy;

import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

/// How far an image is from the expected one, from the error of each pixel: |found - expected| for [#measure], or
/// the number of differing bits for [#measureBits]
public class ImageDifference {
    /// Largest error
    public double maxAbs;
    /// Largest |found - expected|/max(1, |expected|). Zero for [#measureBits]
    public double maxRel;
    /// Root mean square of the errors
    public double rms;
    /// Largest distance in representable values. For integer images it's the largest error
    public long maxUlp;
    /// Fraction of pixels which aren't identical
    public double differing;
    /// Mean of the errors
    public double meanError;

    private long count, differ;
    private double sum, sumSq;

    /// Errors are |found - expected|
    public static ImageDifference measure( ImageGray<?> expected, ImageGray<?> found ) {
        checkShape(expected, found);
        ImageDifference d = new ImageDifference();
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
                d.add(error, ulp);
                d.maxRel = Math.max(d.maxRel, error/Math.max(1.0, Math.abs(e)));
            }
        }
        return d.finish();
    }

    /// Errors are the number of bits which differ, for images whose pixels are bit masks, e.g. census transforms.
    /// GrayU8, GrayS32 and GrayS64
    public static ImageDifference measureBits( ImageGray<?> expected, ImageGray<?> found ) {
        checkShape(expected, found);
        ImageDifference d = new ImageDifference();
        for (int y = 0; y < expected.height; y++) {
            for (int x = 0; x < expected.width; x++) {
                int bits;
                if (expected instanceof GrayU8) {
                    bits = Integer.bitCount(((GrayU8)expected).get(x, y) ^ ((GrayU8)found).get(x, y));
                } else if (expected instanceof GrayS32) {
                    bits = Integer.bitCount(((GrayS32)expected).get(x, y) ^ ((GrayS32)found).get(x, y));
                } else if (expected instanceof GrayS64) {
                    bits = Long.bitCount(((GrayS64)expected).get(x, y) ^ ((GrayS64)found).get(x, y));
                } else {
                    throw new IllegalArgumentException("Not a bit image: " + expected.getClass().getSimpleName());
                }
                d.add(bits, bits);
            }
        }
        return d.finish();
    }

    private static void checkShape( ImageGray<?> expected, ImageGray<?> found ) {
        if (expected.width != found.width || expected.height != found.height)
            throw new IllegalArgumentException("Different shapes");
    }

    private void add( double error, long ulp ) {
        maxAbs = Math.max(maxAbs, error);
        maxUlp = Math.max(maxUlp, ulp);
        sum += error;
        sumSq += error*error;
        count++;
        if (ulp != 0)
            differ++;
    }

    private ImageDifference finish() {
        rms = count == 0 ? 0 : Math.sqrt(sumSq/count);
        meanError = count == 0 ? 0 : sum/count;
        differing = count == 0 ? 0 : differ/(double)count;
        return this;
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
