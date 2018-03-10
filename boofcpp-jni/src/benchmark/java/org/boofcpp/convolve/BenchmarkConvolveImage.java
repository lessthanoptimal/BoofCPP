package org.boofcpp.convolve;

import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.core.image.border.BorderType;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.core.image.border.ImageBorder;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.io.image.UtilImageIO;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D_S32;
import boofcv.struct.image.GrayS16;
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
public class BenchmarkConvolveImage {

    private static final String IMAGE_PATH="/org/boofcpp/threshold/test_image.jpg";

    GrayU8 input;
    GrayS16 output;

    Kernel1D_S32 kernel1D = FactoryKernelGaussian.gaussian1D(GrayU8.class,-1,5);
    Kernel2D_S32 kernel2D = FactoryKernelGaussian.gaussian2D(GrayU8.class,-1,5);

    ImageBorder<GrayU8> border = FactoryImageBorder.single(GrayU8.class, BorderType.REFLECT);

    NativeConvolveImage nativeOps = new NativeConvolveImage();

    static {
        BoofCPP.loadlib();
    }

    public BenchmarkConvolveImage() {

        URL url = getClass().getResource(IMAGE_PATH);
        if( url == null )
            throw new RuntimeException("Can't find resource! "+IMAGE_PATH);
//        System.out.println("URL path "+url.getFile());
        BufferedImage buffered = UtilImageIO.loadImage(url.getFile());
        if( buffered == null )
            throw new RuntimeException("Couldn't load the image");
        input= ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);
        output = input.createSameShape(GrayS16.class);
    }

    @Benchmark
    public void horizontal_native() {
        nativeOps.horizontal(kernel1D,input,output,border);
    }

    @Benchmark
    public void vertical_native() {
        nativeOps.vertical(kernel1D,input,output,border);
    }

    @Benchmark
    public void convolve_native() {
        nativeOps.convolve(kernel2D,input,output,border);
    }

    @Benchmark
    public void horizontal() {
        GConvolveImageOps.horizontal(kernel1D,input,output,border);
    }

    @Benchmark
    public void vertical() {
        GConvolveImageOps.vertical(kernel1D,input,output,border);
    }

    @Benchmark
    public void convolve() {
        GConvolveImageOps.convolve(kernel2D,input,output,border);
    }
}
