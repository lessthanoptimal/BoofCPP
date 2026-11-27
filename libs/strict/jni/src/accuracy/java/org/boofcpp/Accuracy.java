package org.boofcpp;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS8;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.accuracy.AccuracyRecord;
import org.boofcpp.accuracy.AverageDownSampleAccuracy;
import org.boofcpp.accuracy.CensusAccuracy;
import org.boofcpp.accuracy.DisparityAccuracy;
import org.boofcpp.accuracy.MeanBlurAccuracy;
import org.boofcpp.accuracy.ThresholdAccuracy;
import org.boofcpp.convolve.NativeImageBlurOps;
import org.boofcpp.disparity.NativeDisparityBlockMatch;
import org.boofcpp.misc.NativeAverageDownSampleOps;
import org.boofcpp.threshold.NativeBlockMean;
import org.boofcpp.threshold.NativeBlockMinMax;
import org.boofcpp.threshold.NativeBlockOtsu;
import org.boofcpp.threshold.NativeGlobalOtsu;
import org.boofcpp.threshold.NativeLocalMean;
import org.boofcpp.transform.NativeCensusTransform;

/// Strict's accuracy against BoofCV's Java code. Every metric should be zero
public class Accuracy {
    public static void main( String[] args ) throws Exception {
        BoofCPPStrict.loadlib();
        AccuracyRecord.run("strict", BoofCPPStrict.nativeBuildInfo(),
                new AverageDownSampleAccuracy(new NativeAverageDownSampleOps(), GrayU8.class, GrayS8.class,
                        GrayU16.class, GrayS16.class, GrayS32.class, GrayF32.class, GrayF64.class),
                new MeanBlurAccuracy(new NativeImageBlurOps(), GrayU8.class, GrayU16.class, GrayF32.class,
                        GrayF64.class),
                new CensusAccuracy(new NativeCensusTransform<>(), GrayU8.class, GrayU16.class, GrayF32.class),
                new DisparityAccuracy(NativeDisparityBlockMatch::create, GrayU8.class, GrayF32.class),
                new ThresholdAccuracy(NativeGlobalOtsu::new, NativeLocalMean::new, NativeBlockMean::new,
                        NativeBlockOtsu::new, NativeBlockMinMax::new, GrayU8.class, GrayF32.class));
    }
}
