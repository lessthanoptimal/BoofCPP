package org.boofcpp.fast;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import org.boofcpp.NativeVariant;
import org.boofcpp.fast.convolve.NativeImageBlurOps;
import org.boofcpp.fast.disparity.NativeDisparityBlockMatch;

/// Registers fast's hooks. Each one replaces the registered hook only for the images fast supports
public class FastVariant implements NativeVariant {
    @Override public int priority() {
        return 100;
    }

    @Override public boolean isSupported() {
        return BoofCPPFast.isLoaded();
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    @Override public void initialize() {
        BOverrideBlurImageOps.mean = new NativeImageBlurOps(BOverrideBlurImageOps.mean);
        BOverrideStereoDisparity.opCreateBlockMatch = NativeDisparityBlockMatch::create;
    }
}
