package org.boofcpp.benchmark.misc;

import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.ImageDataType;
import boofcv.struct.image.ImageGray;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

import java.util.Random;

/// Average down-sampling to any size with BoofCV's AverageDownSampleOps, of a uniform random image
@Warmup(iterations = 2, time = 1)
@Measurement(iterations = 3, time = 1)
@State(Scope.Benchmark)
public class AverageDownSampleGeneralBenchmark extends ImplementationBenchmark {
    @Param({"U8", "U16", "F32", "F64"})
    public String type;

    @Param({"500", "333"})
    public int outputSize;

    @Param({"false", "true"})
    public boolean centered;

    @Param({"1000"})
    public int size;

    ImageGray input, output, middle;

    @Setup public void setup( BenchmarkParams params ) {
        setupImplementation(params);
        ImageDataType dataType = ImageDataType.valueOf(type);
        input = GeneralizedImageOps.createSingleBand(dataType, size, size);
        GImageMiscOps.fillUniform(input, new Random(234), 0, 200);
        output = GeneralizedImageOps.createSingleBand(dataType, outputSize, outputSize);
        middle = dataType == ImageDataType.F64 ? new GrayF64(outputSize, size) : new GrayF32(outputSize, size);
        checkHandled(BOverrideAverageDownSampleOps.invokeNativeDown(input, centered, output, middle));
    }

    @Benchmark @Hook(type = BOverrideAverageDownSampleOps.class, field = "general")
    public void down_general() { AverageDownSampleOps.down(input, centered, output, middle); }
}
