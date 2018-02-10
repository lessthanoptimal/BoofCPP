package org.boofcpp;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.convolve.BOverrideConvolveNormalized;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import cz.adamh.utils.NativeUtils;
import org.boofcpp.convolve.NativeImageBlurOps;
import org.boofcpp.convolve.NativeImageConvolveNormalized;
import org.boofcpp.threshold.*;

import java.io.File;

public class BoofCPP {

    static {
        NativeUtils.setLibraryName("JNIBoofCPP");

        // First try loading it locally from the devepmental patj
        if( !NativeUtils.loadLocalPath(new File("build/jni"))) {
            if( !NativeUtils.loadLocalPath(new File("../build/jni"))) {
                // Now try loading it from the jar
                if( !NativeUtils.loadLibraryFromJar("/") ) {
                    throw new RuntimeException("Can't load native libraries");
                }
            }
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

        BOverrideConvolveNormalized.horizontal = new NativeImageConvolveNormalized();
        BOverrideConvolveNormalized.vertical = new NativeImageConvolveNormalized();

    }
}
