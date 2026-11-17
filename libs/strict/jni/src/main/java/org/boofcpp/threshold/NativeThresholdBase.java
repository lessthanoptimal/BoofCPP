package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;
import org.boofcpp.NativeBase;

public abstract class NativeThresholdBase <T extends ImageGray<T>> extends NativeBase<T>
        implements InputToBinary<T>
{
    /// Native supports GrayU8 and GrayF32. Other types throw an IllegalArgumentException, which makes BoofCV use its
    /// Java filter
    public NativeThresholdBase(Class<T> inputType ) {
        super(inputType);
        if (inputType != GrayU8.class && inputType != GrayF32.class)
            throw new IllegalArgumentException("Unsupported image type " + inputType.getSimpleName());
    }


    @Override
    public ImageType<T> getInputType() {
        return imageType;
    }
}
