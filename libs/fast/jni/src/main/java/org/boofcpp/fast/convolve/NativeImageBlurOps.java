package org.boofcpp.fast.convolve;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.ImageBase;
import org.boofcpp.NativeImageBorder;

/// Fast's native mean blur, without and with a border, for GrayF32 and GrayF64. Everything else goes to the hook it
/// replaced, e.g. strict's. Without one, it throws a RuntimeException, which causes BoofCV to fall back to its Java
/// implementation.
///
/// BOOFCPP_OVERRIDE BOverrideBlurImageOps.mean
public class NativeImageBlurOps<T extends ImageBase<T>> implements BOverrideBlurImageOps.Mean<T> {
    private final BOverrideBlurImageOps.Mean<T> previous;

    /// @param previous The hook it replaces, or null
    public NativeImageBlurOps( BOverrideBlurImageOps.Mean<T> previous ) {
        this.previous = previous;
    }

    @Override
    public void processMeanWeighted( T input, T output, int radiusX, int radiusY, T storage ) {
        if (!(input instanceof GrayF32 || input instanceof GrayF64)) {
            previous().processMeanWeighted(input, output, radiusX, radiusY, storage);
            return;
        }
        if (radiusX <= 0 || radiusY <= 0)
            throw new IllegalArgumentException("Radius must be > 0");

        output.reshape(input.width, input.height);
        if (input instanceof GrayF32) {
            nativeMeanF32((GrayF32)input, (GrayF32)output, radiusX, radiusY);
        } else {
            nativeMeanF64((GrayF64)input, (GrayF64)output, radiusX, radiusY);
        }
    }

    /// The borders NativeImageBorder supports. Others throw a RuntimeException, which causes BoofCV to fall back to
    /// its Java implementation
    @Override
    public void processMeanBorder( T input, T output, int radiusX, int radiusY, ImageBorder<T> border,
                                   T storage ) {
        if (!(input instanceof GrayF32 || input instanceof GrayF64) || border == null) {
            previous().processMeanBorder(input, output, radiusX, radiusY, border, storage);
            return;
        }
        if (radiusX <= 0 || radiusY <= 0)
            throw new IllegalArgumentException("Radius must be > 0");
        NativeImageBorder b = NativeImageBorder.of(border, input);

        output.reshape(input.width, input.height);
        if (input instanceof GrayF32) {
            nativeMeanBorderF32((GrayF32)input, (GrayF32)output, radiusX, radiusY, b.type, b.value);
        } else {
            nativeMeanBorderF64((GrayF64)input, (GrayF64)output, radiusX, radiusY, b.type, b.value);
        }
    }

    private BOverrideBlurImageOps.Mean<T> previous() {
        if (previous == null)
            throw new RuntimeException("Not supported by fast");
        return previous;
    }

    native void nativeMeanF32( GrayF32 input, GrayF32 output, int radiusX, int radiusY );

    native void nativeMeanF64( GrayF64 input, GrayF64 output, int radiusX, int radiusY );

    /// 'type' is the border type, or null if every pixel outside the image has 'value'
    native void nativeMeanBorderF32( GrayF32 input, GrayF32 output, int radiusX, int radiusY,
                                     BorderType type, double value );

    native void nativeMeanBorderF64( GrayF64 input, GrayF64 output, int radiusX, int radiusY,
                                     BorderType type, double value );
}
