package org.boofcpp.convolve;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;
import org.boofcpp.NativeBase;
import org.boofcpp.threshold.NativeThresholdBase;

public class NativeImageBlurOps<T extends ImageGray<T>> extends NativeBase<T> implements BOverrideBlurImageOps.Mean<T>
{

    @Override
    public void processMean(T input, T output, int radius, T storage) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");

        setImageType(input.imageType.getImageClass());

        output.reshape(input.width,input.height);
        nativeMean(input,output,radius,storage);
    }

    public native void nativeMean( T input, T output, int radius , T storage );
}
