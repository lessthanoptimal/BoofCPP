package org.boofcpp.misc;

import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.ImageDataType;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPP;
import org.openjdk.jmh.annotations.*;

import java.util.Random;
import java.util.concurrent.TimeUnit;

/// AverageDownSampleOps.down(input, centered, output, middle), where regions can have a non-integer size
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2, time = 1)
@Measurement(iterations = 3, time = 1)
@State(Scope.Benchmark)
@Fork(value=1)
public class BenchmarkAverageDownSampleGeneral {
    @Param({"U8", "U16", "F32", "F64"})
    public String image;

    @Param({"500", "333"})
    public int outputSize;

    @Param({"false", "true"})
    public boolean centered;

    @Param({"1000"})
    public int size;

    ImageGray input, output, middle;

    NativeAverageDownSampleOps nativeOps = new NativeAverageDownSampleOps();

    static {
        BoofCPP.loadlib();
        // The speed target is against single-threaded Java
        BoofConcurrency.USE_CONCURRENT = false;
    }

    @Setup public void setup() {
        ImageDataType type = ImageDataType.valueOf(image);
        input = GeneralizedImageOps.createSingleBand(type, size, size);
        GImageMiscOps.fillUniform(input, new Random(234), 0, 200);
        output = GeneralizedImageOps.createSingleBand(type, outputSize, outputSize);
        middle = type == ImageDataType.F64 ? new GrayF64(outputSize, size) : new GrayF32(outputSize, size);
    }

    @Benchmark
    public void down_native() {
        nativeOps.processDown(input, centered, output, middle);
    }

    @Benchmark
    public void down() {
        AverageDownSampleOps.down(input, centered, output, middle);
    }
}
