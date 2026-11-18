package org.boofcpp.accuracy;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.GrayU8;
import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

public class TestImageDifference extends BoofCPPStandardJUnit {
    @Test
    public void measure_float() {
        GrayF32 expected = new GrayF32(2, 2);
        expected.data = new float[]{0, 1, 100, 4};
        GrayF32 found = expected.clone();
        found.set(0, 1, Math.nextUp(100.0f));
        found.set(1, 1, 4.5f);

        ImageDifference d = ImageDifference.measure(expected, found);
        assertEquals(0.5, d.maxAbs, 1e-12);
        assertEquals(0.5/4, d.maxRel, 1e-12);
        double ulp100 = Math.nextUp(100.0f) - 100.0f;
        assertEquals(Math.sqrt((ulp100*ulp100 + 0.25)/4), d.rms, 1e-12);
        assertEquals(Math.round(0.5/Math.ulp(4.0f)), d.maxUlp);
        assertEquals(0.5, d.differing, 1e-12);
    }

    /// Relative error is absolute below 1
    @Test
    public void measure_relativeBelowOne() {
        GrayF64 expected = new GrayF64(1, 1);
        expected.set(0, 0, 0.001);
        GrayF64 found = new GrayF64(1, 1);
        found.set(0, 0, 0.003);
        assertEquals(0.002, ImageDifference.measure(expected, found).maxRel, 1e-15);
    }

    @Test
    public void measure_integer() {
        GrayU8 expected = new GrayU8(4, 1);
        GrayU8 found = new GrayU8(4, 1);
        found.set(2, 0, 3);
        ImageDifference d = ImageDifference.measure(expected, found);
        assertEquals(3, d.maxAbs);
        assertEquals(3, d.maxUlp);
        assertEquals(0.25, d.differing, 1e-12);
        assertEquals(0.75, d.meanError, 1e-12);
    }

    /// Errors are the number of differing bits, whatever their place
    @Test
    public void measureBits() {
        GrayS32 expected = new GrayS32(4, 1);
        GrayS32 found = new GrayS32(4, 1);
        found.set(0, 0, 1 << 23);
        found.set(1, 0, 0b101);
        ImageDifference d = ImageDifference.measureBits(expected, found);
        assertEquals(2, d.maxAbs);
        assertEquals(2, d.maxUlp);
        assertEquals(0, d.maxRel);
        assertEquals(0.5, d.differing, 1e-12);
        assertEquals(0.75, d.meanError, 1e-12);
        assertEquals(Math.sqrt(5/4.0), d.rms, 1e-12);

        GrayU8 a = new GrayU8(1, 1), b = new GrayU8(1, 1);
        b.set(0, 0, 0xFF);
        assertEquals(8, ImageDifference.measureBits(a, b).meanError, 1e-12);
        GrayS64 c = new GrayS64(1, 1), e = new GrayS64(1, 1);
        e.set(0, 0, -1L);
        assertEquals(64, ImageDifference.measureBits(c, e).meanError, 1e-12);
        assertThrows(IllegalArgumentException.class,
                () -> ImageDifference.measureBits(new GrayF32(1, 1), new GrayF32(1, 1)));
    }

    /// Longs which a double can't tell apart still differ
    @Test
    public void measure_long() {
        GrayS64 expected = new GrayS64(2, 1);
        GrayS64 found = new GrayS64(2, 1);
        expected.set(0, 0, 1L << 60);
        found.set(0, 0, (1L << 60) + 1);
        ImageDifference d = ImageDifference.measure(expected, found);
        assertEquals(1, d.maxUlp);
        assertEquals(0.5, d.differing, 1e-12);
    }

    @Test
    public void measure_differentShapes() {
        assertThrows(IllegalArgumentException.class,
                () -> ImageDifference.measure(new GrayU8(4, 1), new GrayU8(3, 1)));
    }

    @Test
    public void ulpDistance() {
        assertEquals(0, ImageDifference.ulpDistance(0.0f, -0.0f));
        assertEquals(1, ImageDifference.ulpDistance(1.0f, Math.nextUp(1.0f)));
        assertEquals(2, ImageDifference.ulpDistance(Math.nextDown(0.0f), Math.nextUp(0.0f)));
        assertEquals(0, ImageDifference.ulpDistance(0.0, -0.0));
        assertEquals(1, ImageDifference.ulpDistance(-1.0, Math.nextDown(-1.0)));
        assertEquals(2, ImageDifference.ulpDistance(Math.nextDown(0.0), Math.nextUp(0.0)));
        // too far apart for a long
        assertEquals(Long.MAX_VALUE, ImageDifference.ulpDistance(-Double.MAX_VALUE, Double.MAX_VALUE));
    }
}
