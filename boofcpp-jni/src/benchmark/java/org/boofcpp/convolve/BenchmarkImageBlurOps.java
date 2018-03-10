package org.boofcpp.convolve;

import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.io.image.UtilImageIO;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.openjdk.jmh.annotations.*;

import java.awt.image.BufferedImage;
import java.net.URL;
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

    private static final String IMAGE_PATH="/org/boofcpp/threshold/test_image.jpg";

    GrayU8 input;
    GrayU8 output;
    GrayU8 storage;

    int radius = 5;

    NativeImageBlurOps<GrayU8> nativeOps = new NativeImageBlurOps<>();

    static {
        BoofCPP.loadlib();
    }

    public BenchmarkImageBlurOps() {

        URL url = getClass().getResource(IMAGE_PATH);
        if( url == null )
            throw new RuntimeException("Can't find resource! "+IMAGE_PATH);
//        System.out.println("URL path "+url.getFile());
        BufferedImage buffered = UtilImageIO.loadImage(url.getFile());
        if( buffered == null )
            throw new RuntimeException("Couldn't load the image");
        input= ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);
        output = input.createSameShape();
        storage = input.createSameShape();
    }

    @Benchmark
    public void mean_native() {
        nativeOps.processMean(input,output,radius,storage);
    }

    @Benchmark
    public void gaussian_native() {
        nativeOps.processGaussian(input,output,-1,radius,storage);
    }

    @Benchmark
    public void mean() {
        GBlurImageOps.mean(input,output,radius,storage);
    }

    @Benchmark
    public void gaussian() {
        GBlurImageOps.gaussian(input,output,-1,radius,storage);
    }
}
