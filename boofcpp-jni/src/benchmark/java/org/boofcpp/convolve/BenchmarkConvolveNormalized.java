package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.io.image.UtilImageIO;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D_S32;
import boofcv.struct.image.GrayU8;
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
public class BenchmarkConvolveNormalized {

    GrayU8 input;
    GrayU8 output;

    Kernel1D_S32 kernel1D = FactoryKernelGaussian.gaussian1D(GrayU8.class,-1,5);
    Kernel2D_S32 kernel2D = FactoryKernelGaussian.gaussian2D(GrayU8.class,-1,5);

    NativeImageConvolveNormalized nativeOps = new NativeImageConvolveNormalized();

    static {
        BoofCPP.loadlib();
    }

    public BenchmarkConvolveNormalized() {

        BufferedImage buffered = UtilImageIO.loadImage(BenchmarkContour.IMAGE_PATH);
        if( buffered == null )
            throw new RuntimeException("Couldn't load the image");
        input= ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);
        output = input.createSameShape();
    }

    @Benchmark
    public void horizontal_native() {
        nativeOps.horizontal(kernel1D,input,output);
    }

    @Benchmark
    public void vertical_native() {
        nativeOps.vertical(kernel1D,input,output);
    }

//    @Benchmark
//    public void convolve_native() {
//        nativeOps.convolve(kernel2D,input,output,border);
//    }

    @Benchmark
    public void horizontal() {
        GConvolveImageOps.horizontalNormalized(kernel1D,input,output);
    }

    @Benchmark
    public void vertical() {
        GConvolveImageOps.verticalNormalized(kernel1D,input,output);
    }

//    @Benchmark
//    public void convolve() {
//        GConvolveImageOps.convolve(kernel2D,input,output,border);
//    }
}
