package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.concurrency.BoofConcurrency;
import boofcv.struct.border.BorderType;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.ImageBorder;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.convolve.Kernel1D_F32;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D_F32;
import boofcv.struct.convolve.Kernel2D_S32;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayU8;
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
public class BenchmarkConvolveImage {

    /// Threads of native and of BoofCV's Java code. 1 is single threaded, which is the speed target against Java
    @Param({"1", "4"})
    public int threads;

    GrayU8 input;
    GrayS16 output;
    GrayF32 inputF32;
    GrayF32 outputF32;

    Kernel1D_S32 kernel1D = FactoryKernelGaussian.gaussian1D(GrayU8.class,-1,5);
    Kernel2D_S32 kernel2D = FactoryKernelGaussian.gaussian2D(GrayU8.class,-1,5);
    Kernel1D_F32 kernel1D_F32 = FactoryKernelGaussian.gaussian1D(GrayF32.class,-1,5);
    Kernel2D_F32 kernel2D_F32 = FactoryKernelGaussian.gaussian2D(GrayF32.class,-1,5);

    ImageBorder<GrayU8> border = FactoryImageBorder.single(BorderType.EXTENDED, GrayU8.class);
    ImageBorder<GrayF32> borderF32 = FactoryImageBorder.single(BorderType.EXTENDED, GrayF32.class);

    NativeConvolveImage nativeOps = new NativeConvolveImage();

    static {
        BoofCPP.loadlib();
    }

    public BenchmarkConvolveImage() {
        BufferedImage buffered = BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE);
        input= ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);
        output = input.createSameShape(GrayS16.class);
        inputF32 = ConvertBufferedImage.convertFrom(buffered,(GrayF32)null);
        outputF32 = inputF32.createSameShape();
    }

    @Setup public void setThreads() {
        NativeConcurrency.setMaxThreads(threads);
        BoofConcurrency.USE_CONCURRENT = threads > 1;
        BoofConcurrency.setMaxThreads(threads);
    }

    @Benchmark public void horizontal_native_U8() { nativeOps.horizontal(kernel1D,input,output,border); }
    @Benchmark public void vertical_native_U8() { nativeOps.vertical(kernel1D,input,output,border); }
    @Benchmark public void convolve_native_U8() { nativeOps.convolve(kernel2D,input,output,border); }
    @Benchmark public void horizontal_java_U8() { GConvolveImageOps.horizontal(kernel1D,input,output,border); }
    @Benchmark public void vertical_java_U8() { GConvolveImageOps.vertical(kernel1D,input,output,border); }
    @Benchmark public void convolve_java_U8() { GConvolveImageOps.convolve(kernel2D,input,output,border); }

    @Benchmark public void horizontal_native_F32() { nativeOps.horizontal(kernel1D_F32,inputF32,outputF32,borderF32); }
    @Benchmark public void vertical_native_F32() { nativeOps.vertical(kernel1D_F32,inputF32,outputF32,borderF32); }
    @Benchmark public void convolve_native_F32() { nativeOps.convolve(kernel2D_F32,inputF32,outputF32,borderF32); }
    @Benchmark public void horizontal_java_F32() { GConvolveImageOps.horizontal(kernel1D_F32,inputF32,outputF32,borderF32); }
    @Benchmark public void vertical_java_F32() { GConvolveImageOps.vertical(kernel1D_F32,inputF32,outputF32,borderF32); }
    @Benchmark public void convolve_java_F32() { GConvolveImageOps.convolve(kernel2D_F32,inputF32,outputF32,borderF32); }
}
