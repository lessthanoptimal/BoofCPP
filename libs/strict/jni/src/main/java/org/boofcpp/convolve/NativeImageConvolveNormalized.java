package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.ImageBase;
import boofcv.struct.image.*;
import org.boofcpp.NativeBase;

/// Native normalized convolution, called by BoofCV through BOverrideConvolveImageNormalized. Anything native doesn't
/// support throws a RuntimeException, which causes BoofCV to fall back to its Java implementation.
public class NativeImageConvolveNormalized extends NativeBase
        implements
        BOverrideConvolveImageNormalized.Horizontal,
        BOverrideConvolveImageNormalized.Vertical,
        BOverrideConvolveImageNormalized.Convolve
{
    /// Native supports GrayU8 and GrayF32. Others throw a RuntimeException, so BoofCV uses its Java code
    private static void checkTypes( ImageBase input, ImageBase output ) {
        boolean u8 = input instanceof GrayU8 && output instanceof GrayU8;
        boolean f32 = input instanceof GrayF32 && output instanceof GrayF32;
        if (!u8 && !f32)
            throw new RuntimeException("Unsupported image types " + input.getClass().getSimpleName() + " " +
                    output.getClass().getSimpleName());
    }

    @Override
    public void horizontal(Kernel1D kernel, ImageBase input, ImageBase output) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass());
        nativehorizontal(kernel, input, output);
    }
    public native void nativehorizontal( Kernel1D kernel, ImageBase input, ImageBase output );

    @Override
    public void vertical(Kernel1D kernel, ImageBase input, ImageBase output) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass());
        nativevertical(kernel, input, output);
    }

    public native void nativevertical( Kernel1D kernel, ImageBase input, ImageBase output );

    @Override
    public void convolve(Kernel2D kernel, ImageBase input, ImageBase output) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass());
        nativeconvolve(kernel, input, output);
    }

    public native void nativeconvolve( Kernel2D kernel, ImageBase input, ImageBase output );
}
