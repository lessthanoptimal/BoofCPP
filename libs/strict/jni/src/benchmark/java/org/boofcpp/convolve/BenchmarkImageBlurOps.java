package org.boofcpp.convolve;

import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.concurrency.BoofConcurrency;
import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.BenchmarkUtils;
import org.boofcpp.BoofCPP;
import org.boofcpp.NativeConcurrency;
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
    /// Mean with a border reads pixels outside the image from a reflected border
    ImageBorder border;

    int radius = 5;

    NativeImageBlurOps nativeOps = new NativeImageBlurOps();

    /// Threads of native and of BoofCV's Java code. 1 is single threaded, which is the speed target against Java.
    /// Native mean filters of floating point images are single threaded
    @Param({"1", "4"})
    public int threads;

    static {
        BoofCPP.loadlib();
    }

    @Setup public void setup() {
        NativeConcurrency.setMaxThreads(threads);
        BoofConcurrency.USE_CONCURRENT = threads > 1;
        BoofConcurrency.setMaxThreads(threads);

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
        border = FactoryImageBorder.single(BorderType.REFLECT, type);
    }

    @Benchmark
    public void mean_native() {
        nativeOps.processMeanWeighted(input, output, radius, radius, storage);
    }

    @Benchmark
    public void mean_with_border_native() {
        nativeOps.processMeanBorder(input, output, radius, radius, border, storage);
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
    public void mean_with_border() {
        GBlurImageOps.meanB(input, output, radius, radius, border, storage, null);
    }

    @Benchmark
    public void gaussian() {
        GBlurImageOps.gaussian(input, output, -1, radius, storage);
    }
}
