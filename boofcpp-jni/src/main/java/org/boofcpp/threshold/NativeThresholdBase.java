package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;
import org.boofcpp.NativeBase;

public abstract class NativeThresholdBase <T extends ImageGray<T>> extends NativeBase<T>
        implements InputToBinary<T>
{
    public NativeThresholdBase(Class<T> inputType ) {
        super(inputType);
    }


    @Override
    public ImageType<T> getInputType() {
        return imageType;
    }
}
