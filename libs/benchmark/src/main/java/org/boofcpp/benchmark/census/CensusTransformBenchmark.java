package org.boofcpp.benchmark.census;

import boofcv.alg.misc.GImageMiscOps;
import boofcv.alg.transform.census.BOverrideCensusTransform;
import boofcv.alg.transform.census.CensusTransform;
import boofcv.alg.transform.census.GCensusTransform;
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
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.ddogleg.struct.DogArray;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

import java.util.Random;

/// Census transforms with BoofCV's GCensusTransform of a uniform random image, with a reflected border. sample_S64
/// samples a 7x7 block, BoofCV's BLOCK_7_7
@Warmup(iterations = 2, time = 1)
@Measurement(iterations = 3, time = 1)
@State(Scope.Benchmark)
@SuppressWarnings({"rawtypes", "unchecked"})
public class CensusTransformBenchmark extends ImplementationBenchmark {
    @Param({"U8", "U16", "F32"})
    public String type;

    @Param({"2000"})
    public int size;

    ImageGray input;
    ImageBorder border;
    DogArray<Point2D_I32> samples = CensusTransform.createBlockSamples(3);
    GrayU8 dense3x3 = new GrayU8(1, 1);
    GrayS32 dense5x5 = new GrayS32(1, 1);
    GrayS64 sampleS64 = new GrayS64(1, 1);

    @Setup public void setup( BenchmarkParams params ) {
        String op = setupImplementation(params);
        input = GeneralizedImageOps.createSingleBand(ImageDataType.valueOf(type), size, size);
        GImageMiscOps.fillUniform(input, new Random(234), 0, 200);
        border = FactoryImageBorder.single(BorderType.REFLECT, input.getClass());
        dense3x3.reshape(size, size);
        dense5x5.reshape(size, size);
        sampleS64.reshape(size, size);
        switch (op) {
            case "dense3x3":
                checkHandled(BOverrideCensusTransform.invokeNativeDense3x3(input, dense3x3, border));
                break;
            case "dense5x5":
                checkHandled(BOverrideCensusTransform.invokeNativeDense5x5(input, dense5x5, border));
                break;
            case "sample_S64":
                checkHandled(BOverrideCensusTransform.invokeNativeSample_S64(input, samples, sampleS64, border));
                break;
            default: throw new IllegalArgumentException("Unknown op " + op);
        }
    }

    @Benchmark @Hook(type = BOverrideCensusTransform.class, field = "census")
    public void dense3x3() { GCensusTransform.dense3x3(input, dense3x3, border); }

    @Benchmark @Hook(type = BOverrideCensusTransform.class, field = "census")
    public void dense5x5() { GCensusTransform.dense5x5(input, dense5x5, border); }

    @Benchmark @Hook(type = BOverrideCensusTransform.class, field = "census")
    public void sample_S64() { GCensusTransform.sample_S64(input, samples, sampleS64, border, null); }
}
