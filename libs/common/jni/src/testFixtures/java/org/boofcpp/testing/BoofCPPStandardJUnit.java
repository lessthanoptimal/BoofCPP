package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.ImageGray;

import java.util.Random;

/// Every JUnit test class extends this, which TestEnforceCodeStandards checks in each module
public class BoofCPPStandardJUnit {
    protected final Random rand = new Random(23423);

    /// Tolerance of the variant being tested. Strict's results must be identical, so a fast test class sets it
    protected Tolerance tolerance = Tolerance.IDENTICAL;

    /// Results must be within 'tolerance'. See [BoofCPPTesting#assertMatches(ImageGray, ImageGray, Tolerance, double)]
    protected void assertMatches( ImageGray<?> expected, ImageGray<?> found ) {
        BoofCPPTesting.assertMatches(expected, found, tolerance, 1.0);
    }

    /// Results must be within 'tolerance' with the given scale. See CodeStandards.md, Testing Fast, before changing
    /// the scale
    protected void assertMatches( ImageGray<?> expected, ImageGray<?> found, double scale ) {
        BoofCPPTesting.assertMatches(expected, found, tolerance, scale);
    }

    /// Image with uniform random values in [min, max]. It's a sub-image, so its offset and stride aren't the defaults
    protected ImageGray randomSubImage( Class type, int width, int height, double min, double max ) {
        ImageGray input = GeneralizedImageOps.createSingleBand(type, width, height);
        GImageMiscOps.fillUniform(input, rand, min, max);
        return BoofTesting.createSubImageOf(input);
    }
}
