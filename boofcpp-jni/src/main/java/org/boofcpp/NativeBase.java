package org.boofcpp;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;

public abstract class NativeBase<T extends ImageGray<T>>
{
    protected ImageType<T> imageType;

    protected long nativePtr;
    protected boolean isInteger;

    public NativeBase(Class<T> inputType ) {
        setImageType(inputType);
    }

    public NativeBase(){}

    public void setImageType( Class<T> inputType ) {
        this.imageType = ImageType.single(inputType);
        this.isInteger = this.imageType.getDataType().isInteger();
    }
}
