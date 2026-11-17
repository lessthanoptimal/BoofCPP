package org.boofcpp;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.abst.disparity.StereoDisparityBlockMatch;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.filter.binary.BOverrideFactoryBinaryContourFinder;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.override.BOverrideManager;
import boofcv.struct.image.ImageGray;
import org.boofcpp.contour.NativeChang2004;
import org.boofcpp.convolve.NativeConvolveImage;
import org.boofcpp.convolve.NativeImageBlurOps;
import org.boofcpp.convolve.NativeImageConvolveNormalized;
import org.boofcpp.disparity.NativeDisparityBlockMatch;
import org.boofcpp.misc.NativeAverageDownSampleOps;
import org.boofcpp.threshold.*;

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
        // Native BlockMinMax isn't faster than Java's
//        BOverrideFactoryThresholdBinary.blockMinMax = NativeBlockMinMax::new;
        BOverrideFactoryThresholdBinary.blockMean = NativeBlockMean::new;
        BOverrideFactoryThresholdBinary.blockOtsu = NativeBlockOtsu::new;
        BOverrideFactoryThresholdBinary.localMean = NativeLocalMean::new;

        BOverrideBlurImageOps.mean = new NativeImageBlurOps();
        BOverrideBlurImageOps.gaussian = new NativeImageBlurOps();

        BOverrideConvolveImageNormalized.horizontal = new NativeImageConvolveNormalized();
        BOverrideConvolveImageNormalized.vertical = new NativeImageConvolveNormalized();

//        BOverrideConvolveImageMean.horizontal = ?;
//        BOverrideConvolveImageMean.vertical = ?;

        BOverrideConvolveImage.horizontal = new NativeConvolveImage();
        BOverrideConvolveImage.vertical = new NativeConvolveImage();
        BOverrideConvolveImage.convolve = new NativeConvolveImage();

        BOverrideAverageDownSampleOps.square = new NativeAverageDownSampleOps();
        BOverrideAverageDownSampleOps.general = new NativeAverageDownSampleOps();
        BOverrideAverageDownSampleOps.horizontal = new NativeAverageDownSampleOps();
        BOverrideAverageDownSampleOps.vertical = new NativeAverageDownSampleOps();


        BOverrideFactoryBinaryContourFinder.chang2004 = NativeChang2004::new;

        // Returning null means BoofCV uses its Java implementation
        BOverrideStereoDisparity.opCreateBlockMatch = new BOverrideStereoDisparity.CreateBlockMatch() {
            @Override public <T extends ImageGray<T>, DI extends ImageGray<DI>> StereoDisparityBlockMatch<T, DI>
            create( ConfigDisparityBM config, Class<T> imageType, Class<DI> dispType ) {
                if (!NativeDisparityBlockMatch.isSupported(config, imageType))
                    return null;
                return new NativeDisparityBlockMatch<>(config, imageType, dispType);
            }
        };

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
