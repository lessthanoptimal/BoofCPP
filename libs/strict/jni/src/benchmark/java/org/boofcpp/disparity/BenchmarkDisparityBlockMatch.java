package org.boofcpp.disparity;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.ConvertImage;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.NativeConcurrency;
import org.boofcpp.testing.BenchmarkUtils;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

/// Stereo block matching of the stereo pair from BenchmarkUtils.stereoPair() with each error type and input image
/// type. Census uses the default variant.
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
public class BenchmarkDisparityBlockMatch {
    @Param({"SAD", "SSD", "ZSSD", "NCC", "CENSUS"})
    public String error;

    @Param({"U8", "F32"})
    public String image;

    /// Threads of native and of BoofCV's Java code. 1 is single threaded, which is the speed target against Java
    @Param({"1", "4"})
    public int threads;

    ImageGray left, right;

    StereoDisparity java;
    StereoDisparity nativeAlg;

    static {
        BoofCPPStrict.loadlib();
    }

    /// Default configuration, which has subpixel and all checks on
    public static ConfigDisparityBM createConfig( DisparityError error ) {
        var config = new ConfigDisparityBM();
        config.errorType = error;
        return config;
    }

    @Setup public void setup() {
        NativeConcurrency.setMaxThreads(threads);
        BoofConcurrency.USE_CONCURRENT = threads > 1;
        BoofConcurrency.setMaxThreads(threads);
        GrayU8[] pair = BenchmarkUtils.stereoPair();
        if (image.equals("U8")) {
            left = pair[0];
            right = pair[1];
        } else {
            left = ConvertImage.convert(pair[0], (GrayF32)null);
            right = ConvertImage.convert(pair[1], (GrayF32)null);
        }

        ConfigDisparityBM config = createConfig(DisparityError.valueOf(error));
        java = FactoryStereoDisparity.blockMatch(config, left.getClass(), GrayF32.class);
        nativeAlg = new NativeDisparityBlockMatch<>(config, left.getClass(), GrayF32.class);
    }

    @Benchmark public void java() {java.process(left, right);}

    @Benchmark public void nativeAlg() {nativeAlg.process(left, right);}
}
