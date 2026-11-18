package org.boofcpp.fast.disparity;

import boofcv.abst.disparity.StereoDisparityBlockMatch;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.ImageGray;

/// Fast's stereo block matching, for GrayU8 and GrayF32 images. Results are identical to strict's. It's strict's
/// Java class with fast's native code
///
/// BOOFCPP_OVERRIDE BOverrideStereoDisparity.opCreateBlockMatch
public class NativeDisparityBlockMatch<T extends ImageGray<T>, DI extends ImageGray<DI>>
        extends org.boofcpp.disparity.NativeDisparityBlockMatch<T, DI> {
    public NativeDisparityBlockMatch( ConfigDisparityBM config, Class<T> imageType, Class<DI> disparityType ) {
        super(config, imageType, disparityType);
    }

    /// Fast's block matching, or null if the configuration and image type aren't supported, which makes BoofCV use
    /// its Java implementation
    public static <T extends ImageGray<T>, DI extends ImageGray<DI>> StereoDisparityBlockMatch<T, DI>
    create( ConfigDisparityBM config, Class<T> imageType, Class<DI> disparityType ) {
        if (!isSupported(config, imageType))
            return null;
        return new NativeDisparityBlockMatch<>(config, imageType, disparityType);
    }

    @Override protected native void nativeinit( ConfigDisparityBM config, boolean imageF32 );

    @Override protected native void nativedestroy();

    @Override protected native void nativeprocess( T left, T right, DI disparity, GrayF32 score );
}
