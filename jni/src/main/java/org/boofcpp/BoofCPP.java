package org.boofcpp;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.threshold.*;

public class BoofCPP {
    public static void initialize() {
        BOverrideFactoryThresholdBinary.globalFixed = new BOverrideFactoryThresholdBinary.GlobalFixed() {
            @Override
            public <T extends ImageGray<T>> InputToBinary<T> handle(double threshold, boolean down, Class<T> inputType) {
                if( inputType == GrayU8.class) {
                    return (InputToBinary)new NativeGlobalFixed_U8(threshold,down);
                } else if( inputType == GrayF32.class) {
                    return (InputToBinary)new NativeGlobalFixed_F32(threshold,down);
                } else {
                    throw new RuntimeException("Unknown or unsupported image type");
                }
            }
        };

        BOverrideFactoryThresholdBinary.globalOtsu = NativeGlobalOtsu::new;
        BOverrideFactoryThresholdBinary.blockMean = NativeBlockMean::new;
        BOverrideFactoryThresholdBinary.blockOtsu = NativeBlockOtsu::new;
    }
}
