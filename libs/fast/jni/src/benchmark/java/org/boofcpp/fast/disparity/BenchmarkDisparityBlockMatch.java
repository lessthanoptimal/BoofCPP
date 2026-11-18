package org.boofcpp.fast.disparity;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.core.image.ConvertImage;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.factory.transform.census.CensusVariants;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.BenchmarkUtils;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

/// Stereo block matching with each error type: fast through BoofCV, strict's native code and BoofCV's Java code.
/// The stereo pair is from BenchmarkUtils.stereoPair(), and the configuration BoofCV's default (range 100, 7x7 region,
/// subpixel and every check on) with the error type changed
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
@SuppressWarnings({"rawtypes", "unchecked"})
public class BenchmarkDisparityBlockMatch {
    @Param({"SAD", "SSD", "ZSSD", "NCC", "CENSUS_3_3", "CENSUS_5_5", "CENSUS_7_7"})
    public String error;

    @Param({"U8", "F32"})
    public String image;

    ImageGray left, right;
    StereoDisparity fast, strict, java;

    static {
        BoofCPP.loadlib();
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    @Setup public void setup() {
        BoofCPP.initialize();
        GrayU8[] pair = BenchmarkUtils.stereoPair();
        GrayU8 left8 = pair[0], right8 = pair[1];
        Class type = image.equals("U8") ? GrayU8.class : GrayF32.class;
        left = image.equals("U8") ? left8 : ConvertImage.convert(left8, (GrayF32)null);
        right = image.equals("U8") ? right8 : ConvertImage.convert(right8, (GrayF32)null);

        var config = new ConfigDisparityBM();
        if (error.startsWith("CENSUS")) {
            config.errorType = DisparityError.CENSUS;
            config.configCensus.variant = CensusVariants.valueOf("BLOCK" + error.substring(6));
        } else {
            config.errorType = DisparityError.valueOf(error);
        }
        fast = FactoryStereoDisparity.blockMatch(config, type, GrayF32.class);
        strict = new org.boofcpp.disparity.NativeDisparityBlockMatch(config, type, GrayF32.class);
        BOverrideStereoDisparity.CreateBlockMatch hook = BOverrideStereoDisparity.opCreateBlockMatch;
        BOverrideStereoDisparity.opCreateBlockMatch = null;
        java = FactoryStereoDisparity.blockMatch(config, type, GrayF32.class);
        BOverrideStereoDisparity.opCreateBlockMatch = hook;
    }

    @Benchmark public void block_match_fast() { fast.process(left, right); }
    @Benchmark public void block_match_strict() { strict.process(left, right); }
    @Benchmark public void block_match_java() { java.process(left, right); }
}
