package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.ImageBase;
import boofcv.struct.image.ImageType;
import org.boofcpp.NativeBase;

public class NativeImageConvolveNormalized extends NativeBase
        implements
        BOverrideConvolveImageNormalized.Horizontal,
        BOverrideConvolveImageNormalized.Vertical,
        BOverrideConvolveImageNormalized.Convolve
{
    private RuntimeException notSupported = new RuntimeException("Only gray images supported");

    @Override
    public void horizontal(Kernel1D kernel, ImageBase input, ImageBase output) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw notSupported;
        setImageType(input.imageType.getImageClass());
        nativehorizontal(kernel, input, output);
    }
    public native void nativehorizontal( Kernel1D kernel, ImageBase input, ImageBase output );

    @Override
    public void vertical(Kernel1D kernel, ImageBase input, ImageBase output) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw notSupported;
        setImageType(input.imageType.getImageClass());
        nativevertical(kernel, input, output);
    }

    public native void nativevertical( Kernel1D kernel, ImageBase input, ImageBase output );

    @Override
    public void convolve(Kernel2D kernel, ImageBase input, ImageBase output) {
        throw new RuntimeException("Not supported yet!");
//        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
//            throw notSupported;
//        setImageType(input.imageType.getImageClass());
//        nativeconvolve(kernel, input, output);
    }

    public native void nativeconvolve( Kernel2D kernel, ImageBase input, ImageBase output );
}
