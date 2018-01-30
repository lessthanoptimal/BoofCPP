package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageType;

public class NativeGlobalFixed_F32
    implements InputToBinary<GrayF32>
{

    ImageType<GrayF32> imageType = ImageType.single(GrayF32.class);

    long nativePtr;

    public NativeGlobalFixed_F32(double threshold, boolean down) {
        nativeinit(threshold,down);
    }

    public native void nativeinit(double threshold, boolean down);

    public native void nativedestroy();


    @Override
    public void process(GrayF32 input, GrayU8 output) {
        output.reshape(input.width,input.height);
        nativeprocess(input,output);
    }

    public native void nativeprocess(GrayF32 input, GrayU8 output);


    @Override protected void finalize() throws Throwable {
        nativedestroy();
        super.finalize();
    }

    @Override
    public ImageType<GrayF32> getInputType() {
        return imageType;
    }
}
