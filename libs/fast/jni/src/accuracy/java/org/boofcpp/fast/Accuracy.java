package org.boofcpp.fast;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU8;
import org.boofcpp.accuracy.AccuracyRecord;
import org.boofcpp.accuracy.DisparityAccuracy;
import org.boofcpp.accuracy.MeanBlurAccuracy;
import org.boofcpp.accuracy.ThresholdAccuracy;
import org.boofcpp.fast.convolve.NativeImageBlurOps;
import org.boofcpp.fast.disparity.NativeDisparityBlockMatch;
import org.boofcpp.fast.threshold.NativeBlockMean;
import org.boofcpp.fast.threshold.NativeBlockMinMax;
import org.boofcpp.fast.threshold.NativeBlockOtsu;
import org.boofcpp.fast.threshold.NativeLocalMean;

/// Fast's accuracy against BoofCV's Java code
public class Accuracy {
    public static void main( String[] args ) throws Exception {
        if (!BoofCPPFast.isLoaded())
            throw new RuntimeException("Fast's native library didn't load");
        AccuracyRecord.run("fast", BoofCPPFast.nativeBuildInfo(),
                new MeanBlurAccuracy(new NativeImageBlurOps<>(null), GrayF32.class, GrayF64.class),
                new DisparityAccuracy(NativeDisparityBlockMatch::create, GrayU8.class, GrayF32.class),
                new ThresholdAccuracy(null, NativeLocalMean::new, NativeBlockMean::new,
                        NativeBlockOtsu::new, NativeBlockMinMax::new, GrayU8.class, GrayF32.class));
    }
}
