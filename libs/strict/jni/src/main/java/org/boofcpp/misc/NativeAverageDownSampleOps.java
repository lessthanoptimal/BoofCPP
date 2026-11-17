package org.boofcpp.misc;

import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayI16;
import boofcv.struct.image.GrayI8;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS8;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageBase;
import boofcv.struct.image.ImageGray;

/// Native average down sampling, called by BoofCV through BOverrideAverageDownSampleOps. Supports the same gray images
/// as BoofCV's AverageDownSampleOps. Anything native doesn't support throws a RuntimeException, which causes BoofCV to
/// fall back to its Java implementation.
///
/// BOOFCPP_OVERRIDE BOverrideAverageDownSampleOps.square
/// BOOFCPP_OVERRIDE BOverrideAverageDownSampleOps.general
/// BOOFCPP_OVERRIDE BOverrideAverageDownSampleOps.horizontal
/// BOOFCPP_OVERRIDE BOverrideAverageDownSampleOps.vertical
public class NativeAverageDownSampleOps implements BOverrideAverageDownSampleOps.Square,
        BOverrideAverageDownSampleOps.General, BOverrideAverageDownSampleOps.Horizontal,
        BOverrideAverageDownSampleOps.Vertical {
    /// Square regions. The output must already be the down sampled size
    @Override
    public void processDown( ImageGray input, int sampleWidth, ImageGray output ) {
        if (sampleWidth <= 0)
            throw new IllegalArgumentException("sampleWidth must be > 0");

        if (input instanceof GrayU8) {
            nativeDownU8((GrayU8)input, sampleWidth, (GrayI8)output);
        } else if (input instanceof GrayS8) {
            nativeDownS8((GrayS8)input, sampleWidth, (GrayI8)output);
        } else if (input instanceof GrayU16) {
            nativeDownU16((GrayU16)input, sampleWidth, (GrayI16)output);
        } else if (input instanceof GrayS16) {
            nativeDownS16((GrayS16)input, sampleWidth, (GrayI16)output);
        } else if (input instanceof GrayS32) {
            nativeDownS32((GrayS32)input, sampleWidth, (GrayS32)output);
        } else if (input instanceof GrayF32) {
            nativeDownF32((GrayF32)input, sampleWidth, (GrayF32)output);
        } else if (input instanceof GrayF64) {
            nativeDownF64((GrayF64)input, sampleWidth, (GrayF64)output);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
    }

    native void nativeDownU8( GrayU8 input, int sampleWidth, GrayI8 output );

    native void nativeDownS8( GrayS8 input, int sampleWidth, GrayI8 output );

    native void nativeDownU16( GrayU16 input, int sampleWidth, GrayI16 output );

    native void nativeDownS16( GrayS16 input, int sampleWidth, GrayI16 output );

    native void nativeDownS32( GrayS32 input, int sampleWidth, GrayS32 output );

    native void nativeDownF32( GrayF32 input, int sampleWidth, GrayF32 output );

    native void nativeDownF64( GrayF64 input, int sampleWidth, GrayF64 output );

    /// Regions with the size of the output. 'middle' is declared if null, the same as BoofCV
    @Override
    public void processDown( ImageBase input, boolean centered, ImageBase output, ImageGray middle ) {
        if (!(input instanceof ImageGray))
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        if (middle == null) {
            middle = input instanceof GrayF64 ? new GrayF64(output.width, input.height) :
                    new GrayF32(output.width, input.height);
        }

        if (input instanceof GrayU8) {
            nativeGeneralU8((GrayU8)input, centered, (GrayU8)output, (GrayF32)middle);
        } else if (input instanceof GrayU16) {
            nativeGeneralU16((GrayU16)input, centered, (GrayU16)output, (GrayF32)middle);
        } else if (input instanceof GrayF32) {
            nativeGeneralF32((GrayF32)input, centered, (GrayF32)output, (GrayF32)middle);
        } else if (input instanceof GrayF64) {
            nativeGeneralF64((GrayF64)input, centered, (GrayF64)output, (GrayF64)middle);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
    }

    @Override
    public void processHorizontal( ImageBase input, boolean centered, ImageBase output ) {
        if (input instanceof GrayU8) {
            nativeHorizontalU8((GrayU8)input, centered, (GrayF32)output);
        } else if (input instanceof GrayU16) {
            nativeHorizontalU16((GrayU16)input, centered, (GrayF32)output);
        } else if (input instanceof GrayF32) {
            nativeHorizontalF32((GrayF32)input, centered, (GrayF32)output);
        } else if (input instanceof GrayF64) {
            nativeHorizontalF64((GrayF64)input, centered, (GrayF64)output);
        } else {
            throw new RuntimeException("Unsupported image type " + input.getClass().getSimpleName());
        }
    }

    /// The input is GrayF64 for a GrayF64 output and GrayF32 otherwise
    @Override
    public void processVertical( ImageBase input, boolean centered, ImageBase output ) {
        if (output instanceof GrayU8) {
            nativeVerticalU8((GrayF32)input, centered, (GrayU8)output);
        } else if (output instanceof GrayU16) {
            nativeVerticalU16((GrayF32)input, centered, (GrayU16)output);
        } else if (output instanceof GrayF32) {
            nativeVerticalF32((GrayF32)input, centered, (GrayF32)output);
        } else if (output instanceof GrayF64) {
            nativeVerticalF64((GrayF64)input, centered, (GrayF64)output);
        } else {
            throw new RuntimeException("Unsupported image type " + output.getClass().getSimpleName());
        }
    }

    native void nativeGeneralU8( GrayU8 input, boolean centered, GrayU8 output, GrayF32 middle );

    native void nativeGeneralU16( GrayU16 input, boolean centered, GrayU16 output, GrayF32 middle );

    native void nativeGeneralF32( GrayF32 input, boolean centered, GrayF32 output, GrayF32 middle );

    native void nativeGeneralF64( GrayF64 input, boolean centered, GrayF64 output, GrayF64 middle );

    native void nativeHorizontalU8( GrayU8 input, boolean centered, GrayF32 output );

    native void nativeHorizontalU16( GrayU16 input, boolean centered, GrayF32 output );

    native void nativeHorizontalF32( GrayF32 input, boolean centered, GrayF32 output );

    native void nativeHorizontalF64( GrayF64 input, boolean centered, GrayF64 output );

    native void nativeVerticalU8( GrayF32 input, boolean centered, GrayU8 output );

    native void nativeVerticalU16( GrayF32 input, boolean centered, GrayU16 output );

    native void nativeVerticalF32( GrayF32 input, boolean centered, GrayF32 output );

    native void nativeVerticalF64( GrayF64 input, boolean centered, GrayF64 output );
}
