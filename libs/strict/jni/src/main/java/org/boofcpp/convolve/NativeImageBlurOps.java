package org.boofcpp.convolve;

import boofcv.alg.InputSanityCheck;
import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.misc.BoofMiscOps;
import boofcv.struct.ConfigLength;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.border.BorderType;
import boofcv.struct.convolve.Kernel1D_F32;
import boofcv.struct.convolve.Kernel1D_F64;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageBase;
import org.boofcpp.NativeImageBorder;
import org.boofcpp.threshold.NativeThresholdBase;

/// Native mean and Gaussian blur, called by BoofCV through BOverrideBlurImageOps. Supports GrayU8, GrayU16, GrayF32
/// and GrayF64, the same gray images as BoofCV's BlurImageOps. Anything native doesn't support throws a
/// RuntimeException, which causes BoofCV to fall back to its Java implementation.
///
/// BOOFCPP_OVERRIDE BOverrideBlurImageOps.mean
/// BOOFCPP_OVERRIDE BOverrideBlurImageOps.gaussian
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
    /// GrayF64 with the borders NativeImageBorder supports. Anything else throws a RuntimeException, which causes
    /// BoofCV to fall back to its Java implementation.
    @Override
    public void processMeanBorder(T input, T output, int radiusX, int radiusY, ImageBorder<T> border,
                                  T storage) {
        if (border == null)
            throw new RuntimeException("Native requires a border");
        if (radiusX <= 0 || radiusY <= 0)
            throw new IllegalArgumentException("Radius must be > 0");

        NativeImageBorder b = NativeImageBorder.of(border, input);

        output.reshape(input.width, input.height);
        storage = InputSanityCheck.declareOrReshape(input, storage);

        if (input instanceof GrayU8) {
            nativeMeanBorderU8((GrayU8)input, (GrayU8)output, radiusX, radiusY, b.type, b.value, (GrayU8)storage);
        } else if (input instanceof GrayU16) {
            nativeMeanBorderU16((GrayU16)input, (GrayU16)output, radiusX, radiusY, b.type, b.value, (GrayU16)storage);
        } else if (input instanceof GrayF32) {
            nativeMeanBorderF32((GrayF32)input, (GrayF32)output, radiusX, radiusY, b.type, b.value, (GrayF32)storage);
        } else if (input instanceof GrayF64) {
            nativeMeanBorderF64((GrayF64)input, (GrayF64)output, radiusX, radiusY, b.type, b.value, (GrayF64)storage);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
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
