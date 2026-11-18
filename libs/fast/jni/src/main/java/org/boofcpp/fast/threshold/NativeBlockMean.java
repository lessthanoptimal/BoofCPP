package org.boofcpp.fast.threshold;

import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;

/// Fast's native block mean threshold, created by BoofCV's FactoryThresholdBinary through
/// BOverrideFactoryThresholdBinary. Same results as strict's.
///
/// BOOFCPP_OVERRIDE BOverrideFactoryThresholdBinary.blockMean
public class NativeBlockMean<T extends ImageGray<T>> extends org.boofcpp.threshold.NativeBlockMean<T> {
    public NativeBlockMean( ConfigLength regionWidth, double scale, boolean down, boolean thresholdFromLocalBlocks, Class<T> inputType ) {
        super(regionWidth, scale, down, thresholdFromLocalBlocks, inputType);
    }

    @Override public native void nativeprocess( T input, GrayU8 output );

    @Override public native void nativedestroy();

    @Override public native void nativeinit( ConfigLength regionWidth, double scale, boolean down, boolean thresholdFromLocalBlocks );
}
