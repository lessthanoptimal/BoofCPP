package org.boofcpp.convolve;

import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.BenchmarkUtils;
import org.boofcpp.BoofCPP;
import org.openjdk.jmh.annotations.*;

import java.awt.image.BufferedImage;
import java.util.concurrent.TimeUnit;

/**
 * @author Peter Abeles
 */
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value=1)
public class BenchmarkImageBlurOps {
    @Param({"U8", "U16", "F32", "F64"})
    public String image;

    ImageGray input, output, storage;

    int radius = 5;

    NativeImageBlurOps nativeOps = new NativeImageBlurOps();

    static {
        BoofCPP.loadlib();
        // The speed target is against single-threaded Java
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    @Setup public void setup() {
        BufferedImage buffered = BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE);
        GrayU8 gray = ConvertBufferedImage.convertFrom(buffered, (GrayU8)null);

        Class type;
        switch (image) {
            case "U8": type = GrayU8.class; break;
            case "U16": type = GrayU16.class; break;
            case "F32": type = GrayF32.class; break;
            case "F64": type = GrayF64.class; break;
            default: throw new IllegalArgumentException("Unknown image " + image);
        }
        input = GeneralizedImageOps.createSingleBand(type, gray.width, gray.height);
        GConvertImage.convert(gray, input);
        output = (ImageGray)input.createSameShape();
        storage = (ImageGray)input.createSameShape();
    }

    @Benchmark
    public void mean_native() {
        nativeOps.processMeanWeighted(input, output, radius, radius, storage);
    }

    @Benchmark
    public void gaussian_native() {
        nativeOps.processGaussian(input, output, -1, radius, -1, radius, storage);
    }

    @Benchmark
    public void mean() {
        GBlurImageOps.mean(input, output, radius, storage, null);
    }

    @Benchmark
    public void gaussian() {
        GBlurImageOps.gaussian(input, output, -1, radius, storage);
    }
}
