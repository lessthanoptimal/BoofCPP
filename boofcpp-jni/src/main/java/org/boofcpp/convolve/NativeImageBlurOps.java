package org.boofcpp.convolve;

import boofcv.alg.InputSanityCheck;
import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.misc.BoofMiscOps;
import boofcv.struct.ConfigLength;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;
import org.boofcpp.NativeBase;
import org.boofcpp.threshold.NativeThresholdBase;

public class NativeImageBlurOps<T extends ImageGray<T>> extends NativeBase<T>
    implements BOverrideBlurImageOps.Mean<T>,BOverrideBlurImageOps.Gaussian<T>
{

    /**
     * Native only supports the same radius along x and y, which is what BoofCPP supported before. Throwing a
     * RuntimeException causes BoofCV to fall back to its Java implementation.
     */
    @Override
    public void processMeanWeighted(T input, T output, int radiusX, int radiusY, T storage) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");
        if( radiusX != radiusY )
            throw new RuntimeException("Native only supports radiusX == radiusY");
        int radius = radiusX;

        output.reshape(input.width,input.height);
        storage = InputSanityCheck.declareOrReshape(input,storage);

        setImageType(input.imageType.getImageClass());

        nativeMean(input,output,radius,storage);
    }

    public native void nativeMean( T input, T output, int radius , T storage );

    /** Not implemented in native code. BoofCV falls back to Java */
    @Override
    public void processMeanBorder(T input, T output, int radiusX, int radiusY, ImageBorder<T> border, T storage) {
        throw new RuntimeException("Not supported by native code");
    }

    @Override
    public void processGaussian(T input, T output, double sigmaX, int radiusX, double sigmaY, int radiusY, T storage) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");
        if( sigmaX != sigmaY || radiusX != radiusY )
            throw new RuntimeException("Native only supports the same sigma and radius along x and y");
        double sigma = sigmaX;
        int radius = radiusX;

        output.reshape(input.width,input.height);
        storage = InputSanityCheck.declareOrReshape(input,storage);

        setImageType(input.imageType.getImageClass());

        nativeGaussian(input,output,sigma,radius,storage);
    }

    public native void nativeGaussian( T input, T output, double sigma, int radius , T storage );
}
