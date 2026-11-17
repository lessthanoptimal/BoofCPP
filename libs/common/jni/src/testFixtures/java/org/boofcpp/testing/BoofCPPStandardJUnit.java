package org.boofcpp.testing;

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
}
