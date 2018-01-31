package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;

public class NativeGlobalOtsu<T extends ImageGray<T>>
    implements InputToBinary<T>
{

    ImageType<T> imageType;

    long nativePtr;
    boolean isInteger;

    public NativeGlobalOtsu(double minValue, double maxValue, boolean down, Class<T> imageType ) {
        this.imageType = ImageType.single(imageType);
        this.isInteger = this.imageType.getDataType().isInteger();
        nativeinit(minValue,maxValue,down);
    }

    public native void nativeinit(double minValue, double maxValue, boolean down);

    public native void nativedestroy();


    @Override
    public void process(T input, GrayU8 output) {
        output.reshape(input.width,input.height);
        nativeprocess(input,output);
    }

    public native void nativeprocess(T input, GrayU8 output);


    @Override protected void finalize() throws Throwable {
        nativedestroy();
        super.finalize();
    }

    @Override
    public ImageType<T> getInputType() {
        return imageType;
    }
}
