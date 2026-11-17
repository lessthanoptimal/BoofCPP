package org.boofcpp.fast.convolve;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.BenchmarkUtils;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

/// Fast's mean against strict's and BoofCV's single-threaded Java. Fast is called through BoofCV after
/// BoofCPP.initialize(), so if it isn't hooked in, its time is strict's or Java's
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
@SuppressWarnings({"rawtypes", "unchecked"})
public class BenchmarkImageBlurOps {
    @Param({"F32", "F64"})
    public String image;

    ImageGray input, output, storage;

    int radius = 5;

    /// The mean hook BoofCPP.initialize() registered
    BOverrideBlurImageOps.Mean registered;
    org.boofcpp.convolve.NativeImageBlurOps strict = new org.boofcpp.convolve.NativeImageBlurOps();

    static {
        BoofCPP.loadlib();
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    @Setup public void setup() {
        BoofCPP.initialize();
        registered = BOverrideBlurImageOps.mean;

        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        Class type = image.equals("F32") ? GrayF32.class : GrayF64.class;
        input = GeneralizedImageOps.createSingleBand(type, gray.width, gray.height);
        GConvertImage.convert(gray, input);
        output = (ImageGray)input.createSameShape();
        storage = (ImageGray)input.createSameShape();
    }

    @Benchmark public void mean_fast() {
        BOverrideBlurImageOps.mean = registered;
        GBlurImageOps.mean(input, output, radius, storage, null);
    }

    @Benchmark public void mean_strict() {
        strict.processMeanWeighted(input, output, radius, radius, storage);
    }

    @Benchmark public void mean_java() {
        BOverrideBlurImageOps.mean = null;
        GBlurImageOps.mean(input, output, radius, storage, null);
    }
}
