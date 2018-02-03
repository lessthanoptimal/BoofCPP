package org.boofcpp;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import cz.adamh.utils.NativeUtils;
import org.boofcpp.threshold.*;

import java.io.File;
import java.io.IOException;

public class BoofCPP {

    static {
        NativeUtils.setLibraryName("JNIBoofCPP");

        // First try loading it locally from the devepmental patj
        if( !NativeUtils.loadLocalPath(new File("build/jni"))) {
            if( !NativeUtils.loadLocalPath(new File("../build/jni"))) {
                // Now try loading it from the jar
                if( !NativeUtils.loadLibraryFromJar("/natives/") ) {
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
    }
}
