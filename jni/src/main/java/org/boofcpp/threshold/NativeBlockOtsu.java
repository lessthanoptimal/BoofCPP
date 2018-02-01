package org.boofcpp.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

public class NativeBlockOtsu<T extends ImageGray<T>> extends NativeThresholdBase<T>
{

    public NativeBlockOtsu(boolean otsu2, ConfigLength regionWidth , double tuning, double scale,
                           boolean down, boolean thresholdFromLocalBlocks,  Class<T> inputType) {
        super(inputType);
        nativeinit(otsu2, regionWidth, tuning, scale, down, thresholdFromLocalBlocks);
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
    public native void nativeinit( boolean otsu2, ConfigLength regionWidth , double tuning, double scale,
                                   boolean down, boolean thresholdFromLocalBlocks);
}
