package org.boofcpp;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.threshold.*;

public class BoofCPP {

    static {

    }

    public static void initialize() {
        BOverrideFactoryThresholdBinary.globalFixed = NativeGlobalFixed::new;
        BOverrideFactoryThresholdBinary.globalOtsu = NativeGlobalOtsu::new;
        BOverrideFactoryThresholdBinary.blockMean = NativeBlockMean::new;
        BOverrideFactoryThresholdBinary.blockOtsu = NativeBlockOtsu::new;
    }
}
