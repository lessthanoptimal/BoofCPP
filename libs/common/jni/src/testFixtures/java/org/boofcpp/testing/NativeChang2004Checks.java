package org.boofcpp.testing;

import boofcv.abst.filter.binary.BinaryLabelContourFinder;
import boofcv.abst.filter.binary.BinaryLabelContourFinderChang2004;
import boofcv.alg.filter.binary.ContourPacked;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.alg.misc.ImageMiscOps;
import boofcv.factory.filter.binary.BOverrideFactoryBinaryContourFinder;
import boofcv.factory.filter.binary.FactoryBinaryContourFinder;
import boofcv.struct.ConfigLength;
import boofcv.struct.ConnectRule;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayU8;
import georegression.struct.point.Point2D_I32;
import org.ddogleg.struct.DogArray;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/// Checks of a variant's Chang2004 contour finder against BoofCV's, shared by every variant. A variant's test class
/// extends this and passes its hook
public abstract class NativeChang2004Checks extends BoofCPPStandardJUnit {
    private final BOverrideFactoryBinaryContourFinder.Chang2004 hook;
    GrayU8 binary = new GrayU8(400,450);

    protected NativeChang2004Checks( BOverrideFactoryBinaryContourFinder.Chang2004 hook ) {
        this.hook = hook;
        GImageMiscOps.fillUniform(binary,rand,0,1);

        ImageMiscOps.fillRectangle(binary,0,100,80,150,150);
        ImageMiscOps.fillRectangle(binary,1,130,100,60,60);
    }

    @Test public void defaultSettings() {
        BinaryLabelContourFinder alg = hook.createChang2004();

        assertEquals(ConnectRule.FOUR,alg.getConnectRule());
        assertTrue(alg.isSaveInternalContours());
        assertEquals(0,alg.getMinContour(null).computeNegMaxI(100));
        assertEquals(Integer.MAX_VALUE,alg.getMaxContour(null).computeNegMaxI(100));
    }

    @Test public void changing_settings() {
        BinaryLabelContourFinder alg = hook.createChang2004();

        alg.setConnectRule(ConnectRule.EIGHT);
        assertEquals(ConnectRule.EIGHT,alg.getConnectRule());

        alg.setSaveInnerContour(false);
        assertFalse(alg.isSaveInternalContours());

        alg.setMinContour(ConfigLength.fixed(24));
        assertEquals(24,alg.getMinContour(null).computeNegMaxI(100));

        alg.setMaxContour(ConfigLength.fixed(1045));
        assertEquals(1045,alg.getMaxContour(null).computeNegMaxI(100));
    }

    /// Contours can be read and written after processing an image
    @Test public void handle_image() {
        BinaryLabelContourFinder alg = hook.createChang2004();

        GrayS32 labeled = binary.createSameShape(GrayS32.class);

        alg.process(binary,labeled);

        List<ContourPacked> contours = alg.getContours();

        assertTrue(contours.size()>0);

        DogArray<Point2D_I32> storage = new DogArray<>(Point2D_I32::new);
        alg.loadContour(0,storage);

        assertTrue(storage.size>0);

        for (int i = 0; i < storage.size; i++) {
            storage.get(i).setTo(i,i+1);
        }
        int before = storage.size;

        alg.writeContour(0,storage.toList());

        storage = new DogArray<>(Point2D_I32::new);
        alg.loadContour(0,storage);
        assertEquals(before,storage.size);
        for (int i = 0; i < storage.size; i++) {
            assertTrue(storage.get(i).distance(i,i+1)==0);
        }
    }

    /// BoofCV's factory creates the variant's contour finder when the hook is registered
    @Test public void factoryUsesHook() {
        BinaryLabelContourFinder found;
        try {
            BOverrideFactoryBinaryContourFinder.chang2004 = hook;
            found = FactoryBinaryContourFinder.linearChang2004();
        } finally {
            BOverrideFactoryBinaryContourFinder.chang2004 = null;
        }
        assertEquals(hook.createChang2004().getClass(), found.getClass());
    }

    /// Identical labels and contours to BoofCV's Java implementation, with and without inner contours
    @Test public void compareToJava() {
        BinaryLabelContourFinder algNative = hook.createChang2004();
        BinaryLabelContourFinder algJava = new BinaryLabelContourFinderChang2004();

        GrayS32 labeledNative = binary.createSameShape(GrayS32.class);
        GrayS32 labeledJava = binary.createSameShape(GrayS32.class);

        for (boolean saveInner : new boolean[]{true, false})
        for( ConnectRule cr : new ConnectRule[]{ConnectRule.FOUR,ConnectRule.EIGHT}) {
            algNative.setSaveInnerContour(saveInner);
            algJava.setSaveInnerContour(saveInner);
            algNative.setConnectRule(cr);
            algJava.setConnectRule(cr);

            // includes ConfigLength relative to image size
            for (ConfigLength min : new ConfigLength[]{ConfigLength.fixed(0), ConfigLength.fixed(15), ConfigLength.relative(0.03,0)}) {
                algNative.setMinContour(min);
                algJava.setMinContour(min);
                for( ConfigLength max : new ConfigLength[]{ConfigLength.fixed(Integer.MAX_VALUE), ConfigLength.fixed(-1), ConfigLength.fixed(50), ConfigLength.relative(0.2,0)}) {
                    algNative.setMaxContour(max);
                    algJava.setMaxContour(max);

                    compare(algNative, algJava, labeledNative, labeledJava);
                }
            }
        }
    }

    private void compare(BinaryLabelContourFinder algNative, BinaryLabelContourFinder algJava, GrayS32 labeledNative, GrayS32 labeledJava) {
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

    private void comparePoints( BinaryLabelContourFinder a , BinaryLabelContourFinder b , int which )
    {
        DogArray<Point2D_I32> pointsA = new DogArray<>(Point2D_I32::new);
        DogArray<Point2D_I32> pointsB = new DogArray<>(Point2D_I32::new);

        a.loadContour(which,pointsA);
        b.loadContour(which,pointsB);

        assertEquals(pointsA.size, pointsB.size);

        for (int i = 0; i < pointsA.size; i++) {
            assertTrue(pointsA.get(i).distance(pointsB.get(i))==0);
        }

    }
}