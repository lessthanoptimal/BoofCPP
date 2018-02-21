package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;

public class NativeBlockMean<T extends ImageGray<T>> extends NativeThresholdBase<T>
{

    public NativeBlockMean( ConfigLength regionWidth, double scale , boolean down, boolean thresholdFromLocalBlocks,
                            Class<T> inputType ) {
        super(inputType);
        nativeinit(regionWidth,scale,down,thresholdFromLocalBlocks);
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
    public native void nativeinit( ConfigLength regionWidth, double scale , boolean down, boolean thresholdFromLocalBlocks);
}
