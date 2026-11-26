package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.struct.border.*;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.ImageBase;
import boofcv.struct.image.*;
import org.boofcpp.NativeBase;

/// Native convolution, called by BoofCV through BOverrideConvolveImage. Anything native doesn't support throws a
/// RuntimeException, which causes BoofCV to fall back to its Java implementation.
///
/// BOOFCPP_OVERRIDE BOverrideConvolveImage.horizontal
/// BOOFCPP_OVERRIDE BOverrideConvolveImage.vertical
/// BOOFCPP_OVERRIDE BOverrideConvolveImage.convolve
public class NativeConvolveImage extends NativeBase implements
        BOverrideConvolveImage.Horizontal,
        BOverrideConvolveImage.Vertical,
        BOverrideConvolveImage.Convolve
{
    /// Native supports GrayU8 input with GrayI16 or GrayS32 output, and GrayF32 input and output. Others throw a
    /// RuntimeException, so BoofCV uses its Java code
    private static void checkTypes( ImageBase input, ImageBase output ) {
        boolean u8 = input instanceof GrayU8 && (output instanceof GrayI16 || output instanceof GrayS32);
        boolean f32 = input instanceof GrayF32 && output instanceof GrayF32;
        if (!u8 && !f32)
            throw new RuntimeException("Unsupported image types " + input.getClass().getSimpleName() + " " +
                    output.getClass().getSimpleName());
    }

    @Override
    public void convolve(Kernel2D kernel, ImageBase input, ImageBase output, BorderType border) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass(), output.imageType.getImageClass());

        output.reshape(input.width,input.height);

        nativeConvolve(kernel,input,output,border.ordinal());
    }

    @Override
    public void horizontal(Kernel1D kernel, ImageBase input, ImageBase output, BorderType border) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass(), output.imageType.getImageClass());

        output.reshape(input.width,input.height);

        nativeHorizontal(kernel,input,output,border.ordinal());
    }

    @Override
    public void vertical(Kernel1D kernel, ImageBase input, ImageBase output, BorderType border) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass(), output.imageType.getImageClass());

        output.reshape(input.width,input.height);

        nativeVertical(kernel,input,output,border.ordinal());
    }

    public native void nativeConvolve( Kernel2D kernel, ImageBase input, ImageBase output, int borderType );

    public native void nativeHorizontal( Kernel1D kernel, ImageBase input, ImageBase output, int borderType );

    public native void nativeVertical( Kernel1D kernel, ImageBase input, ImageBase output, int borderType );

}
