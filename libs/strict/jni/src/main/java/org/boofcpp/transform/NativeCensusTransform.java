package org.boofcpp.transform;

import boofcv.alg.transform.census.BOverrideCensusTransform;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.InterleavedU16;
import georegression.struct.point.Point2D_I32;
import org.boofcpp.NativeImageBorder;
import org.ddogleg.struct.FastAccess;

/// Native census transform, called by BoofCV through BOverrideCensusTransform. Supports GrayU8, GrayU16 and GrayF32
/// inputs, the borders NativeImageBorder supports, and no border. Anything else, including sample_IU16, throws a
/// RuntimeException, which causes BoofCV to fall back to its Java implementation.
///
/// BOOFCPP_OVERRIDE BOverrideCensusTransform.census
public class NativeCensusTransform<T extends ImageGray<T>> implements BOverrideCensusTransform.Census<T> {
    @Override
    public void dense3x3( T input, GrayU8 output, ImageBorder<T> border ) {
        nativeDense(3, input, output, border);
    }

    @Override
    public void dense5x5( T input, GrayS32 output, ImageBorder<T> border ) {
        nativeDense(5, input, output, border);
    }

    @Override
    public void sample_S64( T input, FastAccess<Point2D_I32> sample, GrayS64 output, ImageBorder<T> border ) {
        int[] xs = new int[sample.size], ys = new int[sample.size];
        for (int i = 0; i < sample.size; i++) {
            xs[i] = sample.get(i).x;
            ys[i] = sample.get(i).y;
        }
        nativeSampleS64(input, xs, ys, output, border);
    }

    @Override
    public void sample_IU16( T input, FastAccess<Point2D_I32> sample, InterleavedU16 output,
                             ImageBorder<T> border ) {
        throw new RuntimeException("Not supported by native");
    }

    /// Dense census of a width x width region. 'border' is null if the image border isn't processed
    private static void nativeDense( int width, ImageGray input, ImageGray output, ImageBorder border ) {
        NativeImageBorder b = border == null ? null : NativeImageBorder.of(border, input);
        boolean hasBorder = b != null;
        BorderType type = hasBorder ? b.type : null;
        double value = hasBorder ? b.value : 0;
        if (input instanceof GrayU8) {
            nativeDenseU8(width, (GrayU8)input, output, hasBorder, type, value);
        } else if (input instanceof GrayU16) {
            nativeDenseU16(width, (GrayU16)input, output, hasBorder, type, value);
        } else if (input instanceof GrayF32) {
            nativeDenseF32(width, (GrayF32)input, output, hasBorder, type, value);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
    }

    private static void nativeSampleS64( ImageGray input, int[] xs, int[] ys, GrayS64 output, ImageBorder border ) {
        NativeImageBorder b = border == null ? null : NativeImageBorder.of(border, input);
        boolean hasBorder = b != null;
        BorderType type = hasBorder ? b.type : null;
        double value = hasBorder ? b.value : 0;
        if (input instanceof GrayU8) {
            nativeSampleS64U8((GrayU8)input, xs, ys, output, hasBorder, type, value);
        } else if (input instanceof GrayU16) {
            nativeSampleS64U16((GrayU16)input, xs, ys, output, hasBorder, type, value);
        } else if (input instanceof GrayF32) {
            nativeSampleS64F32((GrayF32)input, xs, ys, output, hasBorder, type, value);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
    }

    // 'type' is the border type, or null if every pixel outside the image has 'value'. 'output' is GrayU8 for width 3
    // and GrayS32 for width 5
    private static native void nativeDenseU8( int width, GrayU8 input, ImageGray output, boolean hasBorder,
                                              BorderType type, double value );

    private static native void nativeDenseU16( int width, GrayU16 input, ImageGray output, boolean hasBorder,
                                               BorderType type, double value );

    private static native void nativeDenseF32( int width, GrayF32 input, ImageGray output, boolean hasBorder,
                                               BorderType type, double value );

    private static native void nativeSampleS64U8( GrayU8 input, int[] xs, int[] ys, GrayS64 output,
                                                  boolean hasBorder, BorderType type, double value );

    private static native void nativeSampleS64U16( GrayU16 input, int[] xs, int[] ys, GrayS64 output,
                                                   boolean hasBorder, BorderType type, double value );

    private static native void nativeSampleS64F32( GrayF32 input, int[] xs, int[] ys, GrayS64 output,
                                                   boolean hasBorder, BorderType type, double value );
}
