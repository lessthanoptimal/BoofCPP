package org.boofcpp.benchmark.disparity;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.factory.transform.census.CensusVariants;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.boofcpp.testing.BenchmarkUtils;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

/// Stereo block matching with each census variant. Same stereo pair and configuration as
/// [DisparityBlockMatchBenchmark]
@State(Scope.Benchmark)
public class DisparityCensusVariantsBenchmark extends ImplementationBenchmark {
    @Param({"BLOCK_3_3", "BLOCK_5_5", "BLOCK_7_7", "BLOCK_9_7", "BLOCK_13_5", "CIRCLE_9"})
    public String variant;

    GrayU8 left, right;
    StereoDisparity<GrayU8, GrayF32> alg;

    @Setup public void setup( BenchmarkParams params ) {
        setupImplementation(params);
        GrayU8[] pair = BenchmarkUtils.stereoPair();
        left = pair[0];
        right = pair[1];
        var config = new ConfigDisparityBM();
        config.errorType = DisparityError.CENSUS;
        config.configCensus.variant = CensusVariants.valueOf(variant);
        alg = checked(FactoryStereoDisparity.blockMatch(config, GrayU8.class, GrayF32.class));
    }

    @Benchmark @Hook(type = BOverrideStereoDisparity.class, field = "opCreateBlockMatch")
    public void block_match_census() { alg.process(left, right); }
}
