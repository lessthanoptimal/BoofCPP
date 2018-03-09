package org.boofcpp.convolve;

import boofcv.alg.InputSanityCheck;
import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.misc.BoofMiscOps;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;
import org.boofcpp.NativeBase;
import org.boofcpp.threshold.NativeThresholdBase;

public class NativeImageBlurOps<T extends ImageGray<T>> extends NativeBase<T>
    implements BOverrideBlurImageOps.Mean<T>,BOverrideBlurImageOps.Gaussian<T>
{

    @Override
    public void processMean(T input, T output, int radius, T storage) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");

        output.reshape(input.width,input.height);
        storage = InputSanityCheck.checkDeclare(input,storage);

        setImageType(input.imageType.getImageClass());

        nativeMean(input,output,radius,storage);
    }

    public native void nativeMean( T input, T output, int radius , T storage );

    @Override
    public void processGaussian(T input, T output, double sigma, int radius, T storage) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");

        output.reshape(input.width,input.height);
        storage = InputSanityCheck.checkDeclare(input,storage);

        setImageType(input.imageType.getImageClass());

        nativeGaussian(input,output,sigma,radius,storage);
    }

    public native void nativeGaussian( T input, T output, double sigma, int radius , T storage );
}
