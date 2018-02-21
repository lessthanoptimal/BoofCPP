package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;

public class NativeGlobalOtsu<T extends ImageGray<T>> extends NativeThresholdBase<T>
{
    public NativeGlobalOtsu(double minValue, double maxValue, boolean down, Class<T> imageType ) {
        super(imageType);
        nativeinit(minValue,maxValue,down);
    }

    @Override
    public void process(T input, GrayU8 output) {
        output.reshape(input.width,input.height);
        nativeprocess(input,output);
    }

    @Override protected void finalize() throws Throwable {
        nativedestroy();
        super.finalize();
    }


    public native void nativeprocess(T input, GrayU8 output);
    public native void nativedestroy();
    public native void nativeinit(double minValue, double maxValue, boolean down);
}
