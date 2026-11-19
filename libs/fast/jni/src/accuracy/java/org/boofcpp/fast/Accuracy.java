package org.boofcpp.fast;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import org.boofcpp.accuracy.AccuracyRecord;
import org.boofcpp.accuracy.MeanBlurAccuracy;
import org.boofcpp.fast.convolve.NativeImageBlurOps;

/// Fast's accuracy against BoofCV's Java code
public class Accuracy {
    public static void main( String[] args ) throws Exception {
        if (!BoofCPPFast.isLoaded())
            throw new RuntimeException("Fast's native library didn't load");
        AccuracyRecord.run("fast", BoofCPPFast.nativeBuildInfo(),
                new MeanBlurAccuracy(new NativeImageBlurOps<>(null), GrayF32.class, GrayF64.class));
    }
}
