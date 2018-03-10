package org.boofcpp;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.factory.filter.binary.BOverrideFactoryBinaryContourFinder;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import cz.adamh.utils.NativeUtils;
import org.boofcpp.contour.NativeChang2004;
import org.boofcpp.convolve.NativeConvolveImage;
import org.boofcpp.convolve.NativeImageBlurOps;
import org.boofcpp.convolve.NativeImageConvolveNormalized;
import org.boofcpp.threshold.*;

import java.io.File;

public class BoofCPP {

    static {
        boolean success = false;

        try {
            System.loadLibrary("JNIBoofCPP");
            success = true;
        } catch( UnsatisfiedLinkError ignore ){}

        try {
            NativeUtils.setLibraryName("JNIBoofCPP");
            // First try loading it locally from the devepmental path
            success = success || NativeUtils.loadLocalPath(new File("build/boofcpp-jni"));
            success = success || NativeUtils.loadLocalPath(new File("../build/boofcpp-jni"));
            success = success || NativeUtils.loadLibraryFromJar("/");
            success = success || NativeUtils.loadLibraryFromJar("/arm64-v8a");
            success = success || NativeUtils.loadLibraryFromJar("/armeabi-v7a");

            if (!success)
                System.err.println("Failed to load native library");
        } catch( Exception e ) {
            e.printStackTrace();
        }
    }

    /**
     * Do nothing function that will cause the static above to be invoked.
     */
    public static void loadlib(){
        // intentionally empty
    }

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

//        BOverrideConvolveImageMean.horizontal = ?;
//        BOverrideConvolveImageMean.vertical = ?;

        BOverrideConvolveImage.horizontal = new NativeConvolveImage();
        BOverrideConvolveImage.vertical = new NativeConvolveImage();
        BOverrideConvolveImage.convolve = new NativeConvolveImage();


        BOverrideFactoryBinaryContourFinder.chang2004 = NativeChang2004::new;
    }
}
