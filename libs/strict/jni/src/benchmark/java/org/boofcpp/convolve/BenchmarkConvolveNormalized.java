package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D_S32;
import boofcv.struct.image.GrayU8;
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
public class BenchmarkConvolveNormalized {

    GrayU8 input;
    GrayU8 output;

    Kernel1D_S32 kernel1D = FactoryKernelGaussian.gaussian1D(GrayU8.class,-1,5);
    Kernel2D_S32 kernel2D = FactoryKernelGaussian.gaussian2D(GrayU8.class,-1,5);

    NativeImageConvolveNormalized nativeOps = new NativeImageConvolveNormalized();

    static {
        BoofCPP.loadlib();
        // The speed target is against single-threaded Java
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    public BenchmarkConvolveNormalized() {

        BufferedImage buffered = BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE);
        input= ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);
        output = input.createSameShape();
    }

    @Benchmark public void horizontal_native_U8() { nativeOps.horizontal(kernel1D,input,output); }
    @Benchmark public void vertical_native_U8() { nativeOps.vertical(kernel1D,input,output); }

//  @Benchmark public void convolve_native_U8() { nativeOps.convolve(kernel2D,input,output,border); }

    @Benchmark public void horizontal_java_U8() { GConvolveImageOps.horizontalNormalized(kernel1D,input,output); }
    @Benchmark public void vertical_java_U8() { GConvolveImageOps.verticalNormalized(kernel1D,input,output); }

//  @Benchmark public void convolve_java_U8() { GConvolveImageOps.convolve(kernel2D,input,output,border); }
}
