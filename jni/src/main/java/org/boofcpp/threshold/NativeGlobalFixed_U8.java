package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageType;

public class NativeGlobalFixed_U8
    implements InputToBinary<GrayU8>
{

    ImageType<GrayU8> imageType = ImageType.single(GrayU8.class);

    long nativePtr;

    public NativeGlobalFixed_U8(double threshold, boolean down) {
        nativeinit(threshold,down);
    }

    public native void nativeinit(double threshold, boolean down);

    public native void nativedestroy();


    @Override
    public native void process(GrayU8 grayU8, GrayU8 output);

    @Override protected void finalize() throws Throwable {
        nativedestroy();
        super.finalize();
    }

    @Override
    public ImageType<GrayU8> getInputType() {
        return imageType;
    }
}
