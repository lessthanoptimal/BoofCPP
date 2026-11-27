package org.boofcpp.benchmark.misc;

import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.ImageDataType;
import boofcv.struct.image.ImageGray;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

import java.util.Random;

/// Average down-sampling by a square block with BoofCV's AverageDownSampleOps, of a uniform random image. Signed
/// images have pixels with both signs
@Warmup(iterations = 2, time = 1)
@Measurement(iterations = 3, time = 1)
@State(Scope.Benchmark)
public class AverageDownSampleBenchmark extends ImplementationBenchmark {
    @Param({"U8", "S8", "U16", "S16", "S32", "F32", "F64"})
    public String type;

    /// Width of the square block
    @Param({"2", "3", "8"})
    public int width;

    @Param({"1000"})
    public int size;

    ImageGray input, output;

    @Setup public void setup( BenchmarkParams params ) {
        setupImplementation(params);
        ImageDataType dataType = ImageDataType.valueOf(type);
        input = GeneralizedImageOps.createSingleBand(dataType, size, size);
        GImageMiscOps.fillUniform(input, new Random(234), dataType.isSigned() ? -100 : 0,
                dataType.isSigned() ? 100 : 200);
        int outputSize = AverageDownSampleOps.downSampleSize(size, width);
        output = GeneralizedImageOps.createSingleBand(dataType, outputSize, outputSize);
        checkHandled(BOverrideAverageDownSampleOps.invokeNativeDown(input, width, output));
    }

    @Benchmark @Hook(type = BOverrideAverageDownSampleOps.class, field = "square")
    public void down() { AverageDownSampleOps.down(input, width, output); }
}
