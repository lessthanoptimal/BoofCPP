package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.core.image.border.*;
import boofcv.struct.border.*;
import boofcv.core.image.ImageBorderValue;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.ImageBase;
import boofcv.struct.image.*;
import org.boofcpp.NativeBase;

/**
 * @author Peter Abeles
 */
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
    public void convolve(Kernel2D kernel, ImageBase input, ImageBase output, ImageBorder border) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass(), output.imageType.getImageClass());

        output.reshape(input.width,input.height);

        nativeConvolve(kernel,input,output,borderToType(border).ordinal());
    }

    @Override
    public void horizontal(Kernel1D kernel, ImageBase input, ImageBase output, ImageBorder border) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass(), output.imageType.getImageClass());

        output.reshape(input.width,input.height);

        nativeHorizontal(kernel,input,output,borderToType(border).ordinal());
    }

    @Override
    public void vertical(Kernel1D kernel, ImageBase input, ImageBase output, ImageBorder border) {
        checkTypes(input, output);
        setImageType(input.imageType.getImageClass(), output.imageType.getImageClass());

        output.reshape(input.width,input.height);

        nativeVertical(kernel,input,output,borderToType(border).ordinal());
    }

    public native void nativeConvolve( Kernel2D kernel, ImageBase input, ImageBase output, int borderType );

    public native void nativeHorizontal( Kernel1D kernel, ImageBase input, ImageBase output, int borderType );

    public native void nativeVertical( Kernel1D kernel, ImageBase input, ImageBase output, int borderType );

    public static BorderType borderToType( ImageBorder border ) {
        if( border instanceof ImageBorderValue.Value_F32 ) {
            float v = ((ImageBorderValue.Value_F32)border).value;
            if( v == 0 ) {
                return BorderType.ZERO;
            }
        } else if( border instanceof ImageBorderValue.Value_I ) {
            float v = ((ImageBorderValue.Value_I)border).value;
            if( v == 0 ) {
                return BorderType.ZERO;
            }
        } else if( border instanceof ImageBorder1D) {
            BorderIndex1D row = ((ImageBorder1D)border).getRowWrap();

            if( row instanceof BorderIndex1D_Extend ) {
                return BorderType.EXTENDED;
            } else if( row instanceof BorderIndex1D_Reflect ) {
                return BorderType.REFLECT;
            } else if( row instanceof BorderIndex1D_Wrap ) {
                return BorderType.WRAP;
            }
        }

        throw new RuntimeException("Unknown border type. Class "+border.getClass().getSimpleName());
    }

}
