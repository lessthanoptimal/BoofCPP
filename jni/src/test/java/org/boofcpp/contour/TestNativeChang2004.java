package org.boofcpp.contour;

import boofcv.abst.filter.binary.BinaryContourFinder;
import boofcv.abst.filter.binary.BinaryContourFinderChang2004;
import boofcv.alg.filter.binary.ContourPacked;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.alg.misc.ImageMiscOps;
import boofcv.struct.ConnectRule;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayU8;
import georegression.struct.point.Point2D_I32;
import org.boofcpp.BoofCPP;
import org.ddogleg.struct.FastQueue;
import org.junit.Test;

import java.util.List;
import java.util.Random;

import static org.junit.Assert.*;

/**
 * @author Peter Abeles
 */
public class TestNativeChang2004 {
    Random rand = new Random(23423);

    GrayU8 binary = new GrayU8(400,450);

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

    /**
     * Exercise functions related to image processing
     */
    @Test
    public void handle_image() {
        BinaryContourFinder alg = new NativeChang2004();

        GrayS32 labeled = binary.createSameShape(GrayS32.class);

        alg.process(binary,labeled);

        List<ContourPacked> contours = alg.getContours();

        assertTrue(contours.size()>0);

        FastQueue<Point2D_I32> storage = new FastQueue<>(Point2D_I32.class,true);
        alg.loadContour(0,storage);

        assertTrue(storage.size>0);

        for (int i = 0; i < storage.size; i++) {
            storage.get(i).set(i,i+1);
        }
        int before = storage.size;

        alg.writeContour(0,storage.toList());

        storage = new FastQueue<>(Point2D_I32.class,true);
        alg.loadContour(0,storage);
        assertEquals(before,storage.size);
        for (int i = 0; i < storage.size; i++) {
            assertTrue(storage.get(i).distance(i,i+1)==0);
        }
    }

    /**
     * See if it produces identical results to a java implementation
     */
    @Test
    public void compareToJava() {
        BinaryContourFinder algNative = new NativeChang2004();
        BinaryContourFinder algJava = new BinaryContourFinderChang2004();

        GrayS32 labeledNative = binary.createSameShape(GrayS32.class);
        GrayS32 labeledJava = binary.createSameShape(GrayS32.class);

        for( ConnectRule cr : new ConnectRule[]{ConnectRule.FOUR,ConnectRule.EIGHT}) {
            algNative.setConnectRule(cr);
            algJava.setConnectRule(cr);

            for (int min : new int[]{0,15}) {
                algNative.setMinContour(min);
                algJava.setMinContour(min);
                for( int max : new int[]{Integer.MAX_VALUE,50}) {
                    algNative.setMaxContour(max);
                    algJava.setMaxContour(max);

                    compare(algNative, algJava, labeledNative, labeledJava);
                }
            }
        }
    }

    private void compare(BinaryContourFinder algNative, BinaryContourFinder algJava, GrayS32 labeledNative, GrayS32 labeledJava) {
        algNative.process(binary,labeledNative);
        algJava.process(binary,labeledJava);

        for (int y = 0; y < binary.height; y++) {
            for (int x = 0; x < binary.width; x++) {
                assertEquals(labeledJava.get(x,y),labeledNative.get(x,y));
            }
        }

        List<ContourPacked> packedNative = algNative.getContours();
        List<ContourPacked> packedJava = algJava.getContours();

        assertTrue(packedJava.size()>0);
        assertEquals(packedJava.size(),packedNative.size());


        for (int i = 0; i < packedJava.size(); i++) {
            ContourPacked n = packedNative.get(i);
            ContourPacked a = packedJava.get(i);

            assertEquals(a.id,n.id);
            assertEquals(a.externalIndex,n.externalIndex);
            assertEquals(a.internalIndexes.size,n.internalIndexes.size);

            comparePoints(algJava,algNative,a.externalIndex);

            for (int j = 0; j < a.internalIndexes.size; j++) {
                assertEquals(a.internalIndexes.get(j),n.internalIndexes.get(j));
                comparePoints(algJava,algNative,a.internalIndexes.get(j));
            }
        }
    }

    private void comparePoints( BinaryContourFinder a , BinaryContourFinder b , int which )
    {
        FastQueue<Point2D_I32> pointsA = new FastQueue<>(Point2D_I32.class,true);
        FastQueue<Point2D_I32> pointsB = new FastQueue<>(Point2D_I32.class,true);

        a.loadContour(which,pointsA);
        b.loadContour(which,pointsB);

        assertEquals(pointsA.size, pointsB.size);

        for (int i = 0; i < pointsA.size; i++) {
            assertTrue(pointsA.get(i).distance(pointsB.get(i))==0);
        }

    }
}