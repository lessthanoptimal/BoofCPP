package org.boofcpp.misc;

import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.ImageDataType;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPP;
import org.openjdk.jmh.annotations.*;

import java.util.Random;
import java.util.concurrent.TimeUnit;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2, time = 1)
@Measurement(iterations = 3, time = 1)
@State(Scope.Benchmark)
@Fork(value=1)
public class BenchmarkAverageDownSample {
    @Param({"U8", "S8", "U16", "S16", "S32", "F32", "F64"})
    public String image;

    @Param({"2", "3", "8"})
    public int width;

    @Param({"1000"})
    public int size;

    ImageGray input, output;

    NativeAverageDownSampleOps nativeOps = new NativeAverageDownSampleOps();

    static {
        BoofCPP.loadlib();
        // The speed target is against single-threaded Java
        BoofConcurrency.USE_CONCURRENT = false;
    }

    @Setup public void setup() {
        ImageDataType type = ImageDataType.valueOf(image);
        input = GeneralizedImageOps.createSingleBand(type, size, size);
        // signed images have pixels with both signs
        GImageMiscOps.fillUniform(input, new Random(234), type.isSigned() ? -100 : 0, type.isSigned() ? 100 : 200);
        int outputSize = AverageDownSampleOps.downSampleSize(size, width);
        output = GeneralizedImageOps.createSingleBand(type, outputSize, outputSize);
    }

    @Benchmark
    public void down_native() {
        nativeOps.processDown(input, width, output);
    }

    @Benchmark
    public void down() {
        AverageDownSampleOps.down(input, width, output);
    }
}
