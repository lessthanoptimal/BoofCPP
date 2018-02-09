package org.boofcpp.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

public class NativeLocalMean<T extends ImageGray<T>> extends NativeThresholdBase<T>
{

    public NativeLocalMean(ConfigLength regionWidth, double scale , boolean down,
                           Class<T> inputType ) {
        super(inputType);
        nativeinit(regionWidth,scale,down);
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
    public native void nativeinit( ConfigLength regionWidth, double scale , boolean down );
}
