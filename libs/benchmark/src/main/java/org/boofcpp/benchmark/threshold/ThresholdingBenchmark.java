package org.boofcpp.benchmark.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.core.image.ConvertImage;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.boofcpp.testing.BenchmarkUtils;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

/// Global and local thresholds created by BoofCV's FactoryThresholdBinary
@State(Scope.Benchmark)
@SuppressWarnings({"rawtypes", "unchecked"})
public class ThresholdingBenchmark extends ImplementationBenchmark {
    @Param({"U8", "F32"})
    public String type;

    boolean down = true;
    double scale = 0.95;
    ConfigLength regionWidth = ConfigLength.fixed(40);
    boolean localBlocks = false;

    ImageGray input;
    GrayU8 output;
    InputToBinary filter;

    @Setup public void setup( BenchmarkParams params ) {
        String op = setupImplementation(params);
        Class imageType = type.equals("U8") ? GrayU8.class : GrayF32.class;
        filter = checked(create(op, imageType));

        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        input = imageType == GrayU8.class ? gray : ConvertImage.convert(gray, (GrayF32)null);
        output = new GrayU8(input.width, input.height);
    }

    InputToBinary create( String op, Class imageType ) {
        switch (op) {
            case "global_fixed": return FactoryThresholdBinary.globalFixed(125, down, imageType);
            case "global_otsu": return FactoryThresholdBinary.globalOtsu(0, 255, 1.0, down, imageType);
            case "local_mean": return FactoryThresholdBinary.localMean(regionWidth, scale, down, imageType);
            case "block_min_max":
                return FactoryThresholdBinary.blockMinMax(regionWidth, scale, down, localBlocks, 1, imageType);
            case "block_mean": return FactoryThresholdBinary.blockMean(regionWidth, scale, down, localBlocks, imageType);
            case "block_otsu":
                return FactoryThresholdBinary.blockOtsu(regionWidth, scale, down, localBlocks, true, 0, imageType);
            default: throw new IllegalArgumentException("Unknown op " + op);
        }
    }

    @Benchmark @Hook(type = BOverrideFactoryThresholdBinary.class, field = "globalFixed")
    public void global_fixed() { filter.process(input, output); }

    @Benchmark @Hook(type = BOverrideFactoryThresholdBinary.class, field = "globalOtsu")
    public void global_otsu() { filter.process(input, output); }

    @Benchmark @Hook(type = BOverrideFactoryThresholdBinary.class, field = "localMean")
    public void local_mean() { filter.process(input, output); }

    @Benchmark @Hook(type = BOverrideFactoryThresholdBinary.class, field = "blockMinMax")
    public void block_min_max() { filter.process(input, output); }

    @Benchmark @Hook(type = BOverrideFactoryThresholdBinary.class, field = "blockMean")
    public void block_mean() { filter.process(input, output); }

    @Benchmark @Hook(type = BOverrideFactoryThresholdBinary.class, field = "blockOtsu")
    public void block_otsu() { filter.process(input, output); }
}
