package org.boofcpp.disparity;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.concurrency.BoofConcurrency;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.factory.transform.census.CensusVariants;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.NativeConcurrency;
import org.boofcpp.testing.BenchmarkUtils;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

/// Stereo block matching with each census variant. Same stereo pair and configuration as
/// [BenchmarkDisparityBlockMatch].
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
public class BenchmarkDisparityCensusVariants {
    @Param({"BLOCK_3_3", "BLOCK_5_5", "BLOCK_7_7", "BLOCK_9_7", "BLOCK_13_5", "CIRCLE_9"})
    public String variant;

    GrayU8 left, right;

    StereoDisparity<GrayU8, GrayF32> java;
    StereoDisparity<GrayU8, GrayF32> nativeAlg;

    static {
        BoofCPP.loadlib();
        // The speed target is single-threaded native against single-threaded Java
        BoofConcurrency.USE_CONCURRENT = false;
        NativeConcurrency.setMaxThreads(1);
    }

    @Setup public void setup() {
        GrayU8[] pair = BenchmarkUtils.stereoPair();
        left = pair[0];
        right = pair[1];

        ConfigDisparityBM config = BenchmarkDisparityBlockMatch.createConfig(DisparityError.CENSUS);
        config.configCensus.variant = CensusVariants.valueOf(variant);
        java = FactoryStereoDisparity.blockMatch(config, GrayU8.class, GrayF32.class);
        nativeAlg = new NativeDisparityBlockMatch<>(config, GrayU8.class, GrayF32.class);
    }

    @Benchmark public void java() {java.process(left, right);}

    @Benchmark public void nativeAlg() {nativeAlg.process(left, right);}
}
