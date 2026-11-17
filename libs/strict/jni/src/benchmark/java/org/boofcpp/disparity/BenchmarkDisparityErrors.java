package org.boofcpp.disparity;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.core.image.ConvertImage;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.ImageGray;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

/// Stereo block matching with each error type and input image type. Same stereo pair and configuration as
/// [BenchmarkDisparityBlockMatch]. Census uses the default variant.
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
public class BenchmarkDisparityErrors {
    @Param({"SAD", "SSD", "ZSSD", "NCC", "CENSUS"})
    public String error;

    @Param({"U8", "F32"})
    public String image;

    ImageGray left, right;

    StereoDisparity java;
    StereoDisparity nativeAlg;

    @Setup public void setup() {
        // loads the images and the native library, and turns off concurrency
        var shared = new BenchmarkDisparityBlockMatch();
        if (image.equals("U8")) {
            left = shared.left;
            right = shared.right;
        } else {
            left = ConvertImage.convert(shared.left, (GrayF32)null);
            right = ConvertImage.convert(shared.right, (GrayF32)null);
        }

        ConfigDisparityBM config = BenchmarkDisparityBlockMatch.createConfig(DisparityError.valueOf(error));
        java = FactoryStereoDisparity.blockMatch(config, left.getClass(), GrayF32.class);
        nativeAlg = new NativeDisparityBlockMatch<>(config, left.getClass(), GrayF32.class);
    }

    @Benchmark public void java() {java.process(left, right);}

    @Benchmark public void nativeAlg() {nativeAlg.process(left, right);}
}
