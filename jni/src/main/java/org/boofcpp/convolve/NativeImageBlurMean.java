package org.boofcpp.convolve;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;
import org.boofcpp.NativeBase;
import org.boofcpp.threshold.NativeThresholdBase;

public class NativeImageBlurMean<T extends ImageGray<T>> extends NativeBase<T> implements BOverrideBlurImageOps.Mean<T>
{

    public NativeImageBlurMean(Class<T> inputType ) {
        super(inputType);
    }

    @Override
    public void process(T input, T output, int radius, T storage) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");

        output.reshape(input.width,input.height);
        nativeprocess(input,output,radius,storage);
    }

    public native void nativeprocess( T input, T output, int radius , T storage );
}
