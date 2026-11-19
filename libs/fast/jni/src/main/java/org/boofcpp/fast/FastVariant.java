package org.boofcpp.fast;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import org.boofcpp.NativeVariant;
import org.boofcpp.fast.convolve.NativeImageBlurOps;
import org.boofcpp.fast.disparity.NativeDisparityBlockMatch;
import org.boofcpp.fast.threshold.NativeBlockMean;
import org.boofcpp.fast.threshold.NativeBlockMinMax;
import org.boofcpp.fast.threshold.NativeBlockOtsu;
import org.boofcpp.fast.threshold.NativeGlobalOtsu;
import org.boofcpp.fast.threshold.NativeLocalMean;

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
        BOverrideFactoryThresholdBinary.globalOtsu = NativeGlobalOtsu::new;
        BOverrideFactoryThresholdBinary.localMean = NativeLocalMean::new;
        BOverrideFactoryThresholdBinary.blockMean = NativeBlockMean::new;
        BOverrideFactoryThresholdBinary.blockOtsu = NativeBlockOtsu::new;
        BOverrideFactoryThresholdBinary.blockMinMax = NativeBlockMinMax::new;
    }
}
