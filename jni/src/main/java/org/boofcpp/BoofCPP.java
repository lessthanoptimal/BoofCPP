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

    public static final String LIB="JNIBoofCPP";

    static {
        String names[] = new String[]{LIB+".so",LIB+".dll","lib"+LIB+".dylib"};

        if( !loadDevelopmental(names) ) {
            try {
                System.loadLibrary(LIB); // only checks the class path
            } catch (UnsatisfiedLinkError e) {
                boolean success = false;
                for (String s : names) {
                    try {
                        NativeUtils.loadLibraryFromJar("/natives/" + s);
                        success = true;
                    } catch (IOException ignore) {
                    }
                }
                if (!success)
                    throw new RuntimeException("Can't load native library");
            }
        }
    }

    private static boolean loadDevelopmental( String[] names ) {
        for( String n : names ) {
            File f = new File("build/jni/"+n);
            if( f.exists() ) {
                System.load(f.getAbsolutePath());
                return true;
            }
            f = new File("../build/jni/"+n);
            if( f.exists() ) {
                System.load(f.getAbsolutePath());
                return true;
            }
        }
        return false;
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
        BOverrideFactoryThresholdBinary.blockMean = NativeBlockMean::new;
        BOverrideFactoryThresholdBinary.blockOtsu = NativeBlockOtsu::new;
    }
}
