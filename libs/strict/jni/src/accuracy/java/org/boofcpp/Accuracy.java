package org.boofcpp;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.accuracy.AccuracyRecord;
import org.boofcpp.accuracy.MeanBlurAccuracy;
import org.boofcpp.convolve.NativeImageBlurOps;

/// Strict's accuracy against BoofCV's Java code. Every metric should be zero
public class Accuracy {
    public static void main( String[] args ) throws Exception {
        BoofCPP.loadlib();
        AccuracyRecord.run("strict", BoofCPP.nativeBuildInfo(),
                new MeanBlurAccuracy(new NativeImageBlurOps(), GrayU8.class, GrayU16.class, GrayF32.class,
                        GrayF64.class));
    }
}
