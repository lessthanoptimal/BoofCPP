package org.boofcpp.fast.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

/// Fast's native local mean threshold, created by BoofCV's FactoryThresholdBinary through
/// BOverrideFactoryThresholdBinary. Same results as strict's.
///
/// BOOFCPP_OVERRIDE BOverrideFactoryThresholdBinary.localMean
public class NativeLocalMean<T extends ImageGray<T>> extends org.boofcpp.threshold.NativeLocalMean<T> {
    public NativeLocalMean( ConfigLength regionWidth, double scale, boolean down, Class<T> inputType ) {
        super(regionWidth, scale, down, inputType);
    }

    @Override public native void nativeprocess( T input, GrayU8 output );

    @Override public native void nativedestroy();

    @Override public native void nativeinit( ConfigLength regionWidth, double scale, boolean down );
}
