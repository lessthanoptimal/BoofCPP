package org.boofcpp.fast.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

/// Fast's native block Otsu threshold, created by BoofCV's FactoryThresholdBinary through
/// BOverrideFactoryThresholdBinary. Same results as strict's.
///
/// BOOFCPP_OVERRIDE BOverrideFactoryThresholdBinary.blockOtsu
public class NativeBlockOtsu<T extends ImageGray<T>> extends org.boofcpp.threshold.NativeBlockOtsu<T> {
    public NativeBlockOtsu( boolean otsu2, ConfigLength regionWidth, double tuning, double scale, boolean down,
                            boolean thresholdFromLocalBlocks, Class<T> inputType ) {
        super(otsu2, regionWidth, tuning, scale, down, thresholdFromLocalBlocks, inputType);
    }

    @Override public native void nativeprocess( T input, GrayU8 output );

    @Override public native void nativedestroy();

    @Override public native void nativeinit( boolean otsu2, ConfigLength regionWidth, double tuning, double scale,
                                             boolean down, boolean thresholdFromLocalBlocks );
}
