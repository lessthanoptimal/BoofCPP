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
import boofcv.struct.image.ImageGray;

/// Native average down sampling of square regions, called by BoofCV through BOverrideAverageDownSampleOps. Supports
/// the same gray images as BoofCV's AverageDownSampleOps.down(input, sampleWidth, output). The output must already be
/// the down sampled size. Anything native doesn't support throws a RuntimeException, which causes BoofCV to fall back
/// to its Java implementation.
public class NativeAverageDownSampleOps implements BOverrideAverageDownSampleOps.Square {
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
}
