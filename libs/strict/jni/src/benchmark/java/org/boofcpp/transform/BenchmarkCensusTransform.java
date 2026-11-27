package org.boofcpp.transform;

import boofcv.alg.misc.GImageMiscOps;
import boofcv.alg.transform.census.CensusTransform;
import boofcv.alg.transform.census.GCensusTransform;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageDataType;
import boofcv.struct.image.ImageGray;
import georegression.struct.point.Point2D_I32;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.NativeConcurrency;
import org.ddogleg.struct.DogArray;
import org.openjdk.jmh.annotations.*;

import java.util.Random;
import java.util.concurrent.TimeUnit;

/// Census transforms with a reflected border. sample_S64 samples a 7x7 block, BoofCV's BLOCK_7_7
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2, time = 1)
@Measurement(iterations = 3, time = 1)
@State(Scope.Benchmark)
@Fork(value = 1)
@SuppressWarnings({"rawtypes", "unchecked"})
public class BenchmarkCensusTransform {
    @Param({"U8", "U16", "F32"})
    public String image;

    @Param({"2000"})
    public int size;

    /// Threads of native and of BoofCV's Java code. 1 is single threaded, which is the speed target against Java
    @Param({"1", "4"})
    public int threads;

    ImageGray input;
    BorderType borderType = BorderType.REFLECT;
    ImageBorder border;
    DogArray<Point2D_I32> samples = CensusTransform.createBlockSamples(3);
    GrayU8 dense3x3 = new GrayU8(1, 1);
    GrayS32 dense5x5 = new GrayS32(1, 1);
    GrayS64 sampleS64 = new GrayS64(1, 1);

    NativeCensusTransform nativeOps = new NativeCensusTransform();

    static {
        BoofCPPStrict.loadlib();
    }

    @Setup public void setup() {
        NativeConcurrency.setMaxThreads(threads);
        BoofConcurrency.USE_CONCURRENT = threads > 1;
        BoofConcurrency.setMaxThreads(threads);
        input = GeneralizedImageOps.createSingleBand(ImageDataType.valueOf(image), size, size);
        GImageMiscOps.fillUniform(input, new Random(234), 0, 200);
        border = FactoryImageBorder.single(borderType, input.getClass());
        dense3x3.reshape(size, size);
        dense5x5.reshape(size, size);
        sampleS64.reshape(size, size);
    }

    @Benchmark public void dense3x3_native() { nativeOps.dense3x3(input, dense3x3, borderType); }
    @Benchmark public void dense3x3_java() { GCensusTransform.dense3x3(input, dense3x3, border); }
    @Benchmark public void dense5x5_native() { nativeOps.dense5x5(input, dense5x5, borderType); }
    @Benchmark public void dense5x5_java() { GCensusTransform.dense5x5(input, dense5x5, border); }
    @Benchmark public void sample_S64_native() { nativeOps.sample_S64(input, samples, sampleS64, borderType); }
    @Benchmark public void sample_S64_java() { GCensusTransform.sample_S64(input, samples, sampleS64, border, null); }
}
