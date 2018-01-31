package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;

public abstract class NativeThresholdBase <T extends ImageGray<T>>
        implements InputToBinary<T>
{

    ImageType<T> imageType;

    long nativePtr;
    boolean isInteger;

    public NativeThresholdBase(Class<T> inputType ) {
        this.imageType = ImageType.single(inputType);
        this.isInteger = this.imageType.getDataType().isInteger();
    }


    @Override
    public ImageType<T> getInputType() {
        return imageType;
    }
}
