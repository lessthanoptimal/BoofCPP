package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.core.image.border.*;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.ImageBase;
import boofcv.struct.image.ImageType;
import org.boofcpp.NativeBase;

/**
 * @author Peter Abeles
 */
public class NativeConvolveImage extends NativeBase implements
        BOverrideConvolveImage.Horizontal,
        BOverrideConvolveImage.Vertical,
        BOverrideConvolveImage.Convolve
{
    @Override
    public void convolve(Kernel2D kernel, ImageBase input, ImageBase output, ImageBorder border) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");
        setImageType(input.imageType.getImageClass(), output.imageType.getImageClass());

        output.reshape(input.width,input.height);

        nativeConvolve(kernel,input,output,borderToType(border).ordinal());
    }

    @Override
    public void horizontal(Kernel1D kernel, ImageBase input, ImageBase output, ImageBorder border) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");
        setImageType(input.imageType.getImageClass(), output.imageType.getImageClass());

        output.reshape(input.width,input.height);

        nativeHorizontal(kernel,input,output,borderToType(border).ordinal());
    }

    @Override
    public void vertical(Kernel1D kernel, ImageBase input, ImageBase output, ImageBorder border) {
        if( input.getImageType().getFamily() != ImageType.Family.GRAY )
            throw new RuntimeException("Only supports gray images");
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
