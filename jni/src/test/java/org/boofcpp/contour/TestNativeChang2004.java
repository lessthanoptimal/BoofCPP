package org.boofcpp.contour;

import boofcv.alg.misc.GImageMiscOps;
import boofcv.alg.misc.ImageMiscOps;
import boofcv.struct.ConnectRule;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.junit.Test;

import java.util.Random;

import static org.junit.Assert.*;

/**
 * @author Peter Abeles
 */
public class TestNativeChang2004 {
    Random rand = new Random(23423);

    GrayU8 binary = new GrayU8(640,480);

    static {
        BoofCPP.loadlib();
    }

    public TestNativeChang2004() {
        GImageMiscOps.fillUniform(binary,rand,0,1);

        ImageMiscOps.fillRectangle(binary,0,100,80,150,150);
        ImageMiscOps.fillRectangle(binary,1,130,100,60,60);
    }

    @Test
    public void defaultSettings() {
        NativeChang2004 alg = new NativeChang2004();

        assertEquals(ConnectRule.FOUR,alg.getConnectRule());
        assertTrue(alg.isSaveInternalContours());
        assertEquals(0,alg.getMinContour());
        assertEquals(Integer.MAX_VALUE,alg.getMaxContour());
    }

    @Test
    public void changing_settings() {
        NativeChang2004 alg = new NativeChang2004();

        alg.setConnectRule(ConnectRule.EIGHT);
        assertEquals(ConnectRule.EIGHT,alg.getConnectRule());

        alg.setSaveInnerContour(false);
        assertFalse(alg.isSaveInternalContours());

        alg.setMinContour(24);
        assertEquals(24,alg.getMinContour());

        alg.setMaxContour(1045);
        assertEquals(1045,alg.getMaxContour());
    }
}