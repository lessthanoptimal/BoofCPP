package org.boofcpp.fast.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

/// Fast's native block min-max threshold, created by BoofCV's FactoryThresholdBinary through
/// BOverrideFactoryThresholdBinary. Same results as strict's. Strict doesn't register its own, since
/// it isn't faster than BoofCV's Java code
///
/// BOOFCPP_OVERRIDE BOverrideFactoryThresholdBinary.blockMinMax
public class NativeBlockMinMax<T extends ImageGray<T>> extends org.boofcpp.threshold.NativeBlockMinMax<T> {
    public NativeBlockMinMax( ConfigLength regionWidth, double scale, boolean down, double minimumSpread,
                              boolean thresholdFromLocalBlocks, Class<T> imageType ) {
        super(regionWidth, scale, down, minimumSpread, thresholdFromLocalBlocks, imageType);
    }

    @Override public native void nativeprocess( T input, GrayU8 output );

    @Override public native void nativedestroy();

    @Override public native void nativeinit( ConfigLength regionWidth, double scale, boolean down,
                                             double minimumSpread, boolean thresholdFromLocalBlocks );
}
