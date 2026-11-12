package org.boofcpp.convolve;

import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BenchmarkUtils;
import org.boofcpp.BoofCPP;
import org.boofcpp.contour.BenchmarkContour;
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

    GrayU8 input;
    GrayU8 output;
    GrayU8 storage;

    int radius = 5;

    NativeImageBlurOps<GrayU8> nativeOps = new NativeImageBlurOps<>();

    static {
        BoofCPP.loadlib();
        // S0 baseline (BoofCV 0.32) had no concurrency; compare single-threaded Java
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    public BenchmarkImageBlurOps() {

        BufferedImage buffered = BenchmarkUtils.loadImage(BenchmarkContour.IMAGE_PATH);
        input= ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);
        output = input.createSameShape();
        storage = input.createSameShape();
    }

    @Benchmark
    public void mean_native() {
        nativeOps.processMeanWeighted(input,output,radius,radius,storage);
    }

    @Benchmark
    public void gaussian_native() {
        nativeOps.processGaussian(input,output,-1,radius,-1,radius,storage);
    }

    @Benchmark
    public void mean() {
        GBlurImageOps.mean(input,output,radius,storage,null);
    }

    @Benchmark
    public void gaussian() {
        GBlurImageOps.gaussian(input,output,-1,radius,storage);
    }
}
