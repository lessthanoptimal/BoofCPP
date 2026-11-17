package org.boofcpp.convolve;

import boofcv.alg.InputSanityCheck;
import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.misc.BoofMiscOps;
import boofcv.struct.ConfigLength;
import boofcv.core.image.ImageBorderValue;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.core.image.border.BorderIndex1D_Extend;
import boofcv.core.image.border.BorderIndex1D_Reflect;
import boofcv.core.image.border.BorderIndex1D_Wrap;
import boofcv.struct.border.BorderType;
import boofcv.struct.convolve.Kernel1D_F32;
import boofcv.struct.convolve.Kernel1D_F64;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.border.ImageBorder1D;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageBase;
import org.boofcpp.threshold.NativeThresholdBase;

/// Native mean and Gaussian blur, called by BoofCV through BOverrideBlurImageOps. Supports GrayU8, GrayU16, GrayF32
/// and GrayF64, the same gray images as BoofCV's BlurImageOps. Anything native doesn't support throws a
/// RuntimeException, which causes BoofCV to fall back to its Java implementation.
public class NativeImageBlurOps<T extends ImageBase<T>>
    implements BOverrideBlurImageOps.Mean<T>,BOverrideBlurImageOps.Gaussian<T>
{
    @Override
    public void processMeanWeighted(T input, T output, int radiusX, int radiusY, T storage) {
        if (radiusX <= 0 || radiusY <= 0)
            throw new IllegalArgumentException("Radius must be > 0");

        output.reshape(input.width, input.height);
        storage = InputSanityCheck.declareOrReshape(input, storage);

        if (input instanceof GrayU8) {
            nativeMeanU8((GrayU8)input, (GrayU8)output, radiusX, radiusY, (GrayU8)storage);
        } else if (input instanceof GrayU16) {
            nativeMeanU16((GrayU16)input, (GrayU16)output, radiusX, radiusY, (GrayU16)storage);
        } else if (input instanceof GrayF32) {
            nativeMeanF32((GrayF32)input, (GrayF32)output, radiusX, radiusY, (GrayF32)storage);
        } else if (input instanceof GrayF64) {
            nativeMeanF64((GrayF64)input, (GrayF64)output, radiusX, radiusY, (GrayF64)storage);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
    }

    native void nativeMeanU8( GrayU8 input, GrayU8 output, int radiusX, int radiusY, GrayU8 storage );

    native void nativeMeanU16( GrayU16 input, GrayU16 output, int radiusX, int radiusY, GrayU16 storage );

    native void nativeMeanF32( GrayF32 input, GrayF32 output, int radiusX, int radiusY, GrayF32 storage );

    native void nativeMeanF64( GrayF64 input, GrayF64 output, int radiusX, int radiusY, GrayF64 storage );

    /// Mean filter where pixels outside the image are read from 'border'. Native supports GrayU8, GrayU16, GrayF32 and
    /// GrayF64 with a border that's extended, reflected, wrapped, or a constant value. Anything else throws a
    /// RuntimeException, which causes BoofCV to fall back to its Java implementation.
    @Override
    public void processMeanBorder(T input, T output, int radiusX, int radiusY, ImageBorder<T> border,
                                  T storage) {
        if (border == null)
            throw new RuntimeException("Native requires a border");
        if (radiusX <= 0 || radiusY <= 0)
            throw new IllegalArgumentException("Radius must be > 0");

        // null if the border is a constant value
        BorderType type = null;
        double value = 0;
        if (border instanceof ImageBorder1D) {
            type = borderIndexType((ImageBorder1D)border);
        } else if (border instanceof ImageBorderValue.Value_I) {
            value = ((ImageBorderValue.Value_I)border).value;
            int max = input instanceof GrayU8 ? 0xFF : 0xFFFF;
            if (value < 0 || value > max)
                throw new RuntimeException("Border value can't be stored in the image's pixel type");
        } else if (border instanceof ImageBorderValue.Value_F32) {
            value = ((ImageBorderValue.Value_F32)border).value;
        } else if (border instanceof ImageBorderValue.Value_F64) {
            value = ((ImageBorderValue.Value_F64)border).value;
        } else {
            throw new RuntimeException("Unsupported border " + border.getClass().getSimpleName());
        }

        output.reshape(input.width, input.height);
        storage = InputSanityCheck.declareOrReshape(input, storage);

        if (input instanceof GrayU8) {
            nativeMeanBorderU8((GrayU8)input, (GrayU8)output, radiusX, radiusY, type, value, (GrayU8)storage);
        } else if (input instanceof GrayU16) {
            nativeMeanBorderU16((GrayU16)input, (GrayU16)output, radiusX, radiusY, type, value, (GrayU16)storage);
        } else if (input instanceof GrayF32) {
            nativeMeanBorderF32((GrayF32)input, (GrayF32)output, radiusX, radiusY, type, value, (GrayF32)storage);
        } else if (input instanceof GrayF64) {
            nativeMeanBorderF64((GrayF64)input, (GrayF64)output, radiusX, radiusY, type, value, (GrayF64)storage);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
    }

    /// The border type of a border that changes the index of pixels outside the image. The same type must be used
    /// for rows and columns.
    static BorderType borderIndexType( ImageBorder1D border ) {
        Class<?> rowType = border.getRowWrap().getClass();
        if (rowType != border.getColWrap().getClass())
            throw new RuntimeException("Rows and columns must have the same border");
        if (rowType == BorderIndex1D_Extend.class)
            return BorderType.EXTENDED;
        if (rowType == BorderIndex1D_Reflect.class)
            return BorderType.REFLECT;
        if (rowType == BorderIndex1D_Wrap.class)
            return BorderType.WRAP;
        throw new RuntimeException("Unsupported border " + rowType.getSimpleName());
    }

    /// 'type' is the border type, or null if every pixel outside the image has 'value'
    native void nativeMeanBorderU8( GrayU8 input, GrayU8 output, int radiusX, int radiusY,
                                    BorderType type, double value, GrayU8 storage );

    native void nativeMeanBorderU16( GrayU16 input, GrayU16 output, int radiusX, int radiusY,
                                     BorderType type, double value, GrayU16 storage );

    native void nativeMeanBorderF32( GrayF32 input, GrayF32 output, int radiusX, int radiusY,
                                     BorderType type, double value, GrayF32 storage );

    native void nativeMeanBorderF64( GrayF64 input, GrayF64 output, int radiusX, int radiusY,
                                     BorderType type, double value, GrayF64 storage );

    /// The kernels are created by BoofCV's FactoryKernelGaussian, the same as its Java code, and native does the
    /// convolution. A sigma <= 0 is computed from the radius, and a radius <= 0 from the sigma
    @Override
    public void processGaussian(T input, T output, double sigmaX, int radiusX, double sigmaY, int radiusY, T storage) {
        output.reshape(input.width, input.height);
        storage = InputSanityCheck.declareOrReshape(input, storage);

        if (input instanceof GrayU8 || input instanceof GrayU16) {
            Kernel1D_S32 kx = FactoryKernelGaussian.gaussian(Kernel1D_S32.class, sigmaX, radiusX);
            Kernel1D_S32 ky = FactoryKernelGaussian.gaussian(Kernel1D_S32.class, sigmaY, radiusY);
            if (input instanceof GrayU8) {
                nativeGaussianU8((GrayU8)input, (GrayU8)output, kx.data, kx.width, kx.offset,
                        ky.data, ky.width, ky.offset, (GrayU8)storage);
            } else {
                nativeGaussianU16((GrayU16)input, (GrayU16)output, kx.data, kx.width, kx.offset,
                        ky.data, ky.width, ky.offset, (GrayU16)storage);
            }
        } else if (input instanceof GrayF32) {
            Kernel1D_F32 kx = FactoryKernelGaussian.gaussian(Kernel1D_F32.class, sigmaX, radiusX);
            Kernel1D_F32 ky = FactoryKernelGaussian.gaussian(Kernel1D_F32.class, sigmaY, radiusY);
            nativeGaussianF32((GrayF32)input, (GrayF32)output, kx.data, kx.width, kx.offset,
                    ky.data, ky.width, ky.offset, (GrayF32)storage);
        } else if (input instanceof GrayF64) {
            Kernel1D_F64 kx = FactoryKernelGaussian.gaussian(Kernel1D_F64.class, sigmaX, radiusX);
            Kernel1D_F64 ky = FactoryKernelGaussian.gaussian(Kernel1D_F64.class, sigmaY, radiusY);
            nativeGaussianF64((GrayF64)input, (GrayF64)output, kx.data, kx.width, kx.offset,
                    ky.data, ky.width, ky.offset, (GrayF64)storage);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
    }

    native void nativeGaussianU8( GrayU8 input, GrayU8 output, int[] kernelX, int widthX, int offsetX,
                                  int[] kernelY, int widthY, int offsetY, GrayU8 storage );

    native void nativeGaussianU16( GrayU16 input, GrayU16 output, int[] kernelX, int widthX, int offsetX,
                                   int[] kernelY, int widthY, int offsetY, GrayU16 storage );

    native void nativeGaussianF32( GrayF32 input, GrayF32 output, float[] kernelX, int widthX, int offsetX,
                                   float[] kernelY, int widthY, int offsetY, GrayF32 storage );

    native void nativeGaussianF64( GrayF64 input, GrayF64 output, double[] kernelX, int widthX, int offsetX,
                                   double[] kernelY, int widthY, int offsetY, GrayF64 storage );
}
