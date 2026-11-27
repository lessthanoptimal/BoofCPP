package org.boofcpp.benchmark.blur;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.ImageGray;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.Images;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

/// Mean and Gaussian blurs of radius 5 of the test image with BoofCV's GBlurImageOps. mean_with_border reflects the
/// image at its border
@State(Scope.Benchmark)
@SuppressWarnings({"rawtypes", "unchecked"})
public class BlurBenchmark extends ImplementationBenchmark {
    @Param({"U8", "U16", "F32", "F64"})
    public String type;

    int radius = 5;
    ImageGray input, output, storage;
    ImageBorder border;

    @Setup public void setup( BenchmarkParams params ) {
        String op = setupImplementation(params);
        input = Images.testImage(type);
        output = (ImageGray)input.createSameShape();
        storage = (ImageGray)input.createSameShape();
        border = FactoryImageBorder.single(BorderType.REFLECT, input.getClass());
        switch (op) {
            case "mean":
                checkHandled(BOverrideBlurImageOps.invokeNativeMean(input, output, radius, radius, storage));
                break;
            case "mean_with_border":
                checkHandled(BOverrideBlurImageOps.invokeNativeMeanBorder(input, output, radius, radius, border,
                        storage));
                break;
            case "gaussian":
                checkHandled(BOverrideBlurImageOps.invokeNativeGaussian(input, output, -1, radius, -1, radius,
                        storage));
                break;
            default: throw new IllegalArgumentException("Unknown op " + op);
        }
    }

    @Benchmark @Hook(type = BOverrideBlurImageOps.class, field = "mean")
    public void mean() { GBlurImageOps.mean(input, output, radius, storage); }

    @Benchmark @Hook(type = BOverrideBlurImageOps.class, field = "mean")
    public void mean_with_border() { GBlurImageOps.meanB(input, output, radius, radius, border, storage); }

    @Benchmark @Hook(type = BOverrideBlurImageOps.class, field = "gaussian")
    public void gaussian() { GBlurImageOps.gaussian(input, output, -1, radius, storage); }
}
