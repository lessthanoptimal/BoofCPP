package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D_F32;
import boofcv.struct.convolve.Kernel2D_S32;
import boofcv.struct.image.GrayF32;
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
    @Param({"5", "10"})
    public int radius;

    GrayU8 input;
    GrayU8 output;
    GrayF32 inputF32;
    GrayF32 outputF32;

    Kernel1D_S32 kernel1D;
    Kernel2D_S32 kernel2D;
    Kernel2D_F32 kernel2DF32;

    NativeImageConvolveNormalized nativeOps = new NativeImageConvolveNormalized();

    static {
        BoofCPP.loadlib();
        // The speed target is against single-threaded Java
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    @Setup public void setup() {
        BufferedImage buffered = BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE);
        input = ConvertBufferedImage.convertFrom(buffered, (GrayU8)null);
        output = input.createSameShape();
        inputF32 = ConvertBufferedImage.convertFrom(buffered, (GrayF32)null);
        outputF32 = inputF32.createSameShape();
        kernel1D = FactoryKernelGaussian.gaussian1D(GrayU8.class, -1, radius);
        kernel2D = FactoryKernelGaussian.gaussian2D(GrayU8.class, -1, radius);
        kernel2DF32 = FactoryKernelGaussian.gaussian2D(GrayF32.class, -1, radius);
    }

    @Benchmark public void horizontal_native_U8() { nativeOps.horizontal(kernel1D, input, output); }
    @Benchmark public void horizontal_java_U8() { GConvolveImageOps.horizontalNormalized(kernel1D, input, output); }
    @Benchmark public void vertical_native_U8() { nativeOps.vertical(kernel1D, input, output); }
    @Benchmark public void vertical_java_U8() { GConvolveImageOps.verticalNormalized(kernel1D, input, output); }
    @Benchmark public void convolve_native_U8() { nativeOps.convolve(kernel2D, input, output); }
    @Benchmark public void convolve_java_U8() { GConvolveImageOps.convolveNormalized(kernel2D, input, output); }
    @Benchmark public void convolve_native_F32() { nativeOps.convolve(kernel2DF32, inputF32, outputF32); }
    @Benchmark public void convolve_java_F32() { GConvolveImageOps.convolveNormalized(kernel2DF32, inputF32, outputF32); }
}
