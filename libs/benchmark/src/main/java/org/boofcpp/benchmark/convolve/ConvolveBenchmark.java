package org.boofcpp.benchmark.convolve;

import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.Images;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

/// Convolutions of the test image with Gaussian kernels of radius 5 and an extended border, with BoofCV's
/// GConvolveImageOps. U8 images give S16 images
@State(Scope.Benchmark)
@SuppressWarnings({"rawtypes", "unchecked"})
public class ConvolveBenchmark extends ImplementationBenchmark {
    @Param({"U8", "F32"})
    public String type;

    ImageGray input, output;
    Kernel1D kernel1D;
    Kernel2D kernel2D;
    ImageBorder border;

    @Setup public void setup( BenchmarkParams params ) {
        String op = setupImplementation(params);
        input = Images.testImage(type);
        output = input instanceof GrayU8 ? input.createSameShape(GrayS16.class) : (ImageGray)input.createSameShape();
        kernel1D = FactoryKernelGaussian.gaussian1D(input.getClass(), -1, 5);
        kernel2D = FactoryKernelGaussian.gaussian2D(input.getClass(), -1, 5);
        border = FactoryImageBorder.single(BorderType.EXTENDED, input.getClass());
        switch (op) {
            case "horizontal":
                checkHandled(BOverrideConvolveImage.invokeNativeHorizontal(kernel1D, input, output, border));
                break;
            case "vertical":
                checkHandled(BOverrideConvolveImage.invokeNativeVertical(kernel1D, input, output, border));
                break;
            case "convolve":
                checkHandled(BOverrideConvolveImage.invokeNativeConvolve(kernel2D, input, output, border));
                break;
            default: throw new IllegalArgumentException("Unknown op " + op);
        }
    }

    @Benchmark @Hook(type = BOverrideConvolveImage.class, field = "horizontal")
    public void horizontal() { GConvolveImageOps.horizontal(kernel1D, input, output, border); }

    @Benchmark @Hook(type = BOverrideConvolveImage.class, field = "vertical")
    public void vertical() { GConvolveImageOps.vertical(kernel1D, input, output, border); }

    @Benchmark @Hook(type = BOverrideConvolveImage.class, field = "convolve")
    public void convolve() { GConvolveImageOps.convolve(kernel2D, input, output, border); }
}
