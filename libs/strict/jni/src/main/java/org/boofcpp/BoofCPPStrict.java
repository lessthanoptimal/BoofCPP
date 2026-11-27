package org.boofcpp;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.alg.transform.census.BOverrideCensusTransform;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.filter.binary.BOverrideFactoryBinaryContourFinder;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import org.boofcpp.contour.NativeChang2004;
import org.boofcpp.convolve.NativeConvolveImage;
import org.boofcpp.convolve.NativeImageBlurOps;
import org.boofcpp.convolve.NativeImageConvolveNormalized;
import org.boofcpp.disparity.NativeDisparityBlockMatch;
import org.boofcpp.misc.NativeAverageDownSampleOps;
import org.boofcpp.threshold.*;
import org.boofcpp.transform.NativeCensusTransform;

/// Strict: BoofCPP's native code with results identical to BoofCV's Java code
public class BoofCPPStrict {

    static {
        NativeLoader.load("JNIBoofCPP", "boofcpp-jni");
    }

    /**
     * Do nothing function that will cause the static above to be invoked.
     */
    public static void loadlib(){
        // intentionally empty
    }

    /// Compiler, build type and compiler flags of the native library
    public static native String nativeBuildInfo();

    /// Registers the strict hooks with BoofCV, replacing any registered for the same functions. Another variant, e.g.
    /// fast, registered after it replaces only the hooks it has. BoofCV's `BOverrideManager.clear()` removes every hook,
    /// so BoofCV uses its Java code again
    public static void initialize() {
        BOverrideFactoryThresholdBinary.globalFixed = NativeGlobalFixed::new;
        BOverrideFactoryThresholdBinary.globalOtsu = NativeGlobalOtsu::new;
        BOverrideFactoryThresholdBinary.blockMinMax = NativeBlockMinMax::new;
        BOverrideFactoryThresholdBinary.blockMean = NativeBlockMean::new;
        BOverrideFactoryThresholdBinary.blockOtsu = NativeBlockOtsu::new;
        BOverrideFactoryThresholdBinary.localMean = NativeLocalMean::new;

        BOverrideBlurImageOps.mean = new NativeImageBlurOps();
        BOverrideBlurImageOps.gaussian = new NativeImageBlurOps();

        BOverrideConvolveImageNormalized.horizontal = new NativeImageConvolveNormalized();
        BOverrideConvolveImageNormalized.vertical = new NativeImageConvolveNormalized();
        BOverrideConvolveImageNormalized.convolve = new NativeImageConvolveNormalized();

//        BOverrideConvolveImageMean.horizontal = ?;
//        BOverrideConvolveImageMean.vertical = ?;

        BOverrideConvolveImage.horizontal = new NativeConvolveImage();
        BOverrideConvolveImage.vertical = new NativeConvolveImage();
        BOverrideConvolveImage.convolve = new NativeConvolveImage();

        BOverrideAverageDownSampleOps.square = new NativeAverageDownSampleOps();
        BOverrideAverageDownSampleOps.general = new NativeAverageDownSampleOps();
        BOverrideAverageDownSampleOps.horizontal = new NativeAverageDownSampleOps();
        BOverrideAverageDownSampleOps.vertical = new NativeAverageDownSampleOps();

        BOverrideCensusTransform.census = new NativeCensusTransform<>();


        BOverrideFactoryBinaryContourFinder.chang2004 = NativeChang2004::new;

        BOverrideStereoDisparity.opCreateBlockMatch = NativeDisparityBlockMatch::create;
    }
}
