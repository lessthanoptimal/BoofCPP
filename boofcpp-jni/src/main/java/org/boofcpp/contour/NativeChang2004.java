package org.boofcpp.contour;

import boofcv.abst.filter.binary.BinaryContourFinder;
import boofcv.alg.filter.binary.ContourPacked;
import boofcv.struct.ConnectRule;
import boofcv.struct.PackedSetsPoint2D_I32;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayU8;
import georegression.struct.point.Point2D_I32;
import org.ddogleg.struct.FastQueue;
import org.ddogleg.struct.GrowQueue_I32;

import java.util.List;

/**
 * Native java class for Chang 2004 C++ code
 *
 * @author Peter Abeles
 */
public class NativeChang2004 implements BinaryContourFinder {

    // Storage here to simplify JNI wrapper code
    protected FastQueue<ContourPacked> storageContours = new FastQueue<>(ContourPacked.class,true);
    protected PackedSetsPoint2D_I32 packedPoints = new PackedSetsPoint2D_I32(4000);

    // use this to transfer information about points. More memory but many fewer JNI calls and
    // simpler JNI code
    protected GrowQueue_I32 storagePoints = new GrowQueue_I32();

    protected long nativePtr;

    public NativeChang2004() {
        native_init();
    }

    @Override
    public void process(GrayU8 binary, GrayS32 labeled) {
        storageContours.reset();
        packedPoints.reset();
        labeled.reshape(binary.width,binary.height);
        native_process(binary, labeled);
    }

    public native void native_process(GrayU8 binary, GrayS32 labeled);

    protected void addContour( int id , int externalIndex ) {
        ContourPacked P = storageContours.grow();
        P.id = id;
        P.externalIndex = externalIndex;
        P.internalIndexes.setTo(storagePoints);
    }

    @Override
    public List<ContourPacked> getContours() {
        return storageContours.toList();
    }

    @Override
    public void loadContour(int contourID, FastQueue<Point2D_I32> storage) {
        packedPoints.getSet(contourID,storage);
    }

    @Override
    public void writeContour(int contourID, List<Point2D_I32> storage) {
        packedPoints.writeOverSet(contourID,storage);
    }

    @Override
    public native void setSaveInnerContour(boolean enabled);

    @Override
    public native boolean isSaveInternalContours();

    @Override
    public native void setMinContour(int length);

    @Override
    public native int getMinContour();

    @Override
    public native void setMaxContour(int length);

    @Override
    public native int getMaxContour();

    @Override
    public native void setConnectRule(ConnectRule rule);

    @Override
    public native ConnectRule getConnectRule();

    public native void native_init();

    public native void native_destroy();

    @Override protected void finalize() throws Throwable {
        native_destroy();
        super.finalize();
    }
}
