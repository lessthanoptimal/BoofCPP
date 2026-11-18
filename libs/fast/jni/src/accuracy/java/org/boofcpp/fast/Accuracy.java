package org.boofcpp.fast;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.accuracy.AccuracyRecord;
import org.boofcpp.accuracy.CensusAccuracy;
import org.boofcpp.accuracy.MeanBlurAccuracy;
import org.boofcpp.fast.convolve.NativeImageBlurOps;
import org.boofcpp.fast.transform.NativeCensusTransform;

/// Fast's accuracy against BoofCV's Java code
public class Accuracy {
    public static void main( String[] args ) throws Exception {
        if (!BoofCPPFast.isLoaded())
            throw new RuntimeException("Fast's native library didn't load");
        AccuracyRecord.run("fast", BoofCPPFast.nativeBuildInfo(),
                new MeanBlurAccuracy(new NativeImageBlurOps<>(null), GrayF32.class, GrayF64.class),
                new CensusAccuracy(new NativeCensusTransform<>(null), GrayU8.class, GrayU16.class, GrayF32.class));
    }
}
