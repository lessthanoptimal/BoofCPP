package org.boofcpp.contour;

import boofcv.abst.filter.binary.BinaryContourFinder;
import boofcv.abst.filter.binary.BinaryLabelContourFinder;
import boofcv.alg.filter.binary.ContourPacked;
import boofcv.struct.ConfigLength;
import boofcv.struct.ConnectRule;
import boofcv.struct.PackedSetsPoint2D_I32;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayU8;
import georegression.struct.point.Point2D_I32;
import org.ddogleg.struct.DogArray;
import org.ddogleg.struct.DogArray_I32;

import java.util.List;

/**
 * Native java class for Chang 2004 C++ code
 *
 * @author Peter Abeles
 */
public class NativeChang2004 implements BinaryLabelContourFinder {

    // Storage here to simplify JNI wrapper code
    protected DogArray<ContourPacked> storageContours = new DogArray<>(ContourPacked::new);
    protected PackedSetsPoint2D_I32 packedPoints = new PackedSetsPoint2D_I32(4000);

    // use this to transfer information about points. More memory but many fewer JNI calls and
    // simpler JNI code
    protected DogArray_I32 storagePoints = new DogArray_I32();

    protected long nativePtr;

    // Contour length limits. Resolved to pixels for each image, same as LinearContourLabelChang2004
    protected ConfigLength minContour = ConfigLength.fixed(0);
    protected ConfigLength maxContour = ConfigLength.fixed(-1);

    public NativeChang2004() {
        native_init();
    }

    @Override
    public void process(GrayU8 binary, GrayS32 labeled) {
        storageContours.reset();
        packedPoints.reset();
        labeled.reshape(binary.width,binary.height);
        // Uses BoofCV's own ConfigLength math so the threshold in pixels is identical to Java
        double totalLength = Math.sqrt(binary.width*binary.height);
        nativeSetMinContour(minContour.computeNegMaxI(totalLength));
        nativeSetMaxContour(maxContour.computeNegMaxI(totalLength));
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
    public void loadContour(int contourID, DogArray<Point2D_I32> storage) {
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
    public void setMinContour(ConfigLength length) { minContour.setTo(length); }

    @Override
    public ConfigLength getMinContour(ConfigLength length) {
        if (length == null)
            length = new ConfigLength();
        length.setTo(minContour);
        return length;
    }

    @Override
    public void setMaxContour(ConfigLength length) { maxContour.setTo(length); }

    @Override
    public ConfigLength getMaxContour(ConfigLength length) {
        if (length == null)
            length = new ConfigLength();
        length.setTo(maxContour);
        return length;
    }

    public native void nativeSetMinContour(int length);

    public native int nativeGetMinContour();

    public native void nativeSetMaxContour(int length);

    public native int nativeGetMaxContour();

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
