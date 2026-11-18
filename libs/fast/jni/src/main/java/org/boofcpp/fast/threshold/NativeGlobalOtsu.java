package org.boofcpp.fast.threshold;

import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

/// Fast's native global Otsu threshold, created by BoofCV's FactoryThresholdBinary through
/// BOverrideFactoryThresholdBinary. Same results as strict's.
///
/// BOOFCPP_OVERRIDE BOverrideFactoryThresholdBinary.globalOtsu
public class NativeGlobalOtsu<T extends ImageGray<T>> extends org.boofcpp.threshold.NativeGlobalOtsu<T> {
    public NativeGlobalOtsu( double minValue, double maxValue, double scale, boolean down, Class<T> imageType ) {
        super(minValue, maxValue, scale, down, imageType);
    }

    @Override public native void nativeprocess( T input, GrayU8 output );

    @Override public native void nativedestroy();

    @Override public native void nativeinit( double minValue, double maxValue, double scale, boolean down );
}
