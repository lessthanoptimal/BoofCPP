package org.boofcpp.benchmark.convolve;

import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.ImageGray;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.Images;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

/// Normalized convolutions of the test image with Gaussian kernels, with BoofCV's GConvolveImageOps
@State(Scope.Benchmark)
@SuppressWarnings({"rawtypes", "unchecked"})
public class ConvolveNormalizedBenchmark extends ImplementationBenchmark {
    @Param({"U8", "F32"})
    public String type;

    @Param({"5", "10"})
    public int radius;

    ImageGray input, output;
    Kernel1D kernel1D;
    Kernel2D kernel2D;

    @Setup public void setup( BenchmarkParams params ) {
        String op = setupImplementation(params);
        input = Images.testImage(type);
        output = (ImageGray)input.createSameShape();
        kernel1D = FactoryKernelGaussian.gaussian1D(input.getClass(), -1, radius);
        kernel2D = FactoryKernelGaussian.gaussian2D(input.getClass(), -1, radius);
        switch (op) {
            case "horizontal":
                checkHandled(BOverrideConvolveImageNormalized.invokeNativeHorizontal(kernel1D, input, output));
                break;
            case "vertical":
                checkHandled(BOverrideConvolveImageNormalized.invokeNativeVertical(kernel1D, input, output));
                break;
            case "convolve":
                checkHandled(BOverrideConvolveImageNormalized.invokeNativeConvolve(kernel2D, input, output));
                break;
            default: throw new IllegalArgumentException("Unknown op " + op);
        }
    }

    @Benchmark @Hook(type = BOverrideConvolveImageNormalized.class, field = "horizontal")
    public void horizontal() { GConvolveImageOps.horizontalNormalized(kernel1D, input, output); }

    @Benchmark @Hook(type = BOverrideConvolveImageNormalized.class, field = "vertical")
    public void vertical() { GConvolveImageOps.verticalNormalized(kernel1D, input, output); }

    @Benchmark @Hook(type = BOverrideConvolveImageNormalized.class, field = "convolve")
    public void convolve() { GConvolveImageOps.convolveNormalized(kernel2D, input, output); }
}
