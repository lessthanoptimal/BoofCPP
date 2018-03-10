package org.boofcpp;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;

public abstract class NativeBase<T extends ImageGray<T>>
{
    protected ImageType<T> imageType;
    protected ImageType<T> outputType;

    protected long nativePtr;
    protected boolean isInteger;
    protected int inputBits;
    protected int outputBits;

    public NativeBase(Class<T> inputType ) {
        setImageType(inputType);
    }

    public NativeBase(){}

    public void setImageType( Class<T> inputType ) {
        this.imageType = ImageType.single(inputType);
        this.isInteger = this.imageType.getDataType().isInteger();
        this.inputBits = this.imageType.getDataType().getNumBits();
        this.outputBits = this.imageType.getDataType().getNumBits();
    }

    public void setImageType( Class<T> inputType , Class<T> outputType ) {
        this.imageType = ImageType.single(inputType);
        this.outputType = ImageType.single(outputType);
        this.isInteger = this.imageType.getDataType().isInteger();
        this.inputBits = this.imageType.getDataType().getNumBits();
        this.outputBits = this.outputType.getDataType().getNumBits();
    }
}
