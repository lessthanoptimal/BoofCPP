package org.boofcpp;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.alg.transform.census.BOverrideCensusTransform;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.filter.binary.BOverrideFactoryBinaryContourFinder;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.override.BOverrideManager;
import org.boofcpp.contour.NativeChang2004;
import org.boofcpp.convolve.NativeConvolveImage;
import org.boofcpp.convolve.NativeImageBlurOps;
import org.boofcpp.convolve.NativeImageConvolveNormalized;
import org.boofcpp.disparity.NativeDisparityBlockMatch;
import org.boofcpp.misc.NativeAverageDownSampleOps;
import org.boofcpp.threshold.*;
import org.boofcpp.transform.NativeCensusTransform;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.ServiceLoader;

public class BoofCPP {

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

    /// Registers the strict hooks with BoofCV, then the hooks of every other variant that's available, e.g. fast. Each
    /// variant only replaces the hooks it has
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

        for (NativeVariant variant : variants()) {
            variant.initialize();
        }
    }

    /// Variants found by ServiceLoader that can run here, in order of increasing priority
    public static List<NativeVariant> variants() {
        List<NativeVariant> variants = new ArrayList<>();
        for (NativeVariant variant : ServiceLoader.load(NativeVariant.class)) {
            if (variant.isSupported())
                variants.add(variant);
        }
        variants.sort(Comparator.comparingInt(NativeVariant::priority));
        return variants;
    }

    /// Undoes initialize() by removing every BoofCV override, including any not from BoofCPP. BoofCV uses its Java
    /// implementations again
    public static void uninitialize() {
        BOverrideManager.clear();
    }
}
