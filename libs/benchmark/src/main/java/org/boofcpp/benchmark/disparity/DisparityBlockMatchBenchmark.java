package org.boofcpp.benchmark.disparity;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.core.image.ConvertImage;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.boofcpp.testing.BenchmarkUtils;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

/// Stereo block matching with BoofCV's FactoryStereoDisparity of the stereo pair from BenchmarkUtils.stereoPair(),
/// with each error type. The default configuration has subpixel and every check on, and census its default variant
@State(Scope.Benchmark)
@SuppressWarnings({"rawtypes", "unchecked"})
public class DisparityBlockMatchBenchmark extends ImplementationBenchmark {
    @Param({"SAD", "SSD", "ZSSD", "NCC", "CENSUS"})
    public String error;

    @Param({"U8", "F32"})
    public String type;

    ImageGray left, right;
    StereoDisparity alg;

    @Setup public void setup( BenchmarkParams params ) {
        setupImplementation(params);
        GrayU8[] pair = BenchmarkUtils.stereoPair();
        if (type.equals("U8")) {
            left = pair[0];
            right = pair[1];
        } else {
            left = ConvertImage.convert(pair[0], (GrayF32)null);
            right = ConvertImage.convert(pair[1], (GrayF32)null);
        }
        var config = new ConfigDisparityBM();
        config.errorType = DisparityError.valueOf(error);
        alg = checked(FactoryStereoDisparity.blockMatch(config, left.getClass(), GrayF32.class));
    }

    @Benchmark @Hook(type = BOverrideStereoDisparity.class, field = "opCreateBlockMatch")
    public void block_match() { alg.process(left, right); }
}
