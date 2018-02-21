package org.boofcpp.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

public class NativeGlobalFixed<T extends ImageGray<T>> extends NativeThresholdBase<T>
{
    public NativeGlobalFixed(double threshold, boolean down, Class<T> inputType ) {
        super(inputType);
        nativeinit(threshold,down);
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
    public native void nativeinit(double threshold, boolean down);
}
