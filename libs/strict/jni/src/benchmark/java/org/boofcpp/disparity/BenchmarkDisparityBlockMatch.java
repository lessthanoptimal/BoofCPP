package org.boofcpp.disparity;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.testing.BenchmarkUtils;
import org.boofcpp.BoofCPP;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

/// Stereo block matching of the stereo pair from BenchmarkUtils.stereoPair()
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
public class BenchmarkDisparityBlockMatch {
    GrayU8 left, right;

    StereoDisparity<GrayU8, GrayF32> sad_java;
    StereoDisparity<GrayU8, GrayF32> sad_native;
    StereoDisparity<GrayU8, GrayF32> census_java;
    StereoDisparity<GrayU8, GrayF32> census_native;

    static {
        BoofCPP.loadlib();
        // The speed target is against single-threaded Java
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    public BenchmarkDisparityBlockMatch() {
        GrayU8[] pair = BenchmarkUtils.stereoPair();
        left = pair[0];
        right = pair[1];

        sad_java = FactoryStereoDisparity.blockMatch(createConfig(DisparityError.SAD), GrayU8.class, GrayF32.class);
        sad_native = new NativeDisparityBlockMatch<>(createConfig(DisparityError.SAD), GrayU8.class, GrayF32.class);
        census_java = FactoryStereoDisparity.blockMatch(createConfig(DisparityError.CENSUS), GrayU8.class, GrayF32.class);
        census_native = new NativeDisparityBlockMatch<>(createConfig(DisparityError.CENSUS), GrayU8.class, GrayF32.class);
    }

    /// Default configuration, which has subpixel and all checks on
    public static ConfigDisparityBM createConfig( DisparityError error ) {
        var config = new ConfigDisparityBM();
        config.errorType = error;
        return config;
    }

    @Benchmark public void sad_java_U8() { sad_java.process(left, right); }
    @Benchmark public void sad_native_U8() { sad_native.process(left, right); }
    @Benchmark public void census_java_U8() { census_java.process(left, right); }
    @Benchmark public void census_native_U8() { census_native.process(left, right); }
}
