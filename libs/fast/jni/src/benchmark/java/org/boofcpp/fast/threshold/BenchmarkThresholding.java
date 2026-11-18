package org.boofcpp.fast.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.core.image.ConvertImage;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.override.BOverrideManager;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.BenchmarkUtils;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

/// Threshold filters: fast through BoofCV's factory, strict's native code and BoofCV's Java code. Same settings as
/// strict's BenchmarkThresholding: test image, width 40, scale 0.95, down. Ops ending in _local use the local 3x3
/// region of blocks
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
@SuppressWarnings({"rawtypes", "unchecked"})
public class BenchmarkThresholding {
    @Param({"global_otsu", "local_mean", "block_mean", "block_mean_local", "block_otsu", "block_otsu_local",
            "block_minmax", "block_minmax_local"})
    public String op;

    @Param({"U8", "F32"})
    public String image;

    ImageGray input;
    GrayU8 output;
    InputToBinary fast, strict, java;

    static {
        BoofCPP.loadlib();
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    @Setup public void setup() {
        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        Class type = image.equals("U8") ? GrayU8.class : GrayF32.class;
        input = image.equals("U8") ? gray : ConvertImage.convert(gray, (GrayF32)null);
        output = new GrayU8(gray.width, gray.height);

        BoofCPP.initialize();
        fast = create(type, false);
        strict = create(type, true);
        BOverrideManager.clear();
        java = create(type, false);
        BoofCPP.initialize();
    }

    /// The filter from BoofCV's factory, or strict's native filter
    InputToBinary create( Class type, boolean strictNative ) {
        var width = ConfigLength.fixed(40);
        double scale = 0.95;
        boolean down = true;
        boolean local = op.endsWith("_local");
        switch (op.replace("_local", "")) {
            case "global_otsu":
                return strictNative ? new org.boofcpp.threshold.NativeGlobalOtsu(0, 255, 1.0, down, type) :
                        FactoryThresholdBinary.globalOtsu(0, 255, 1.0, down, type);
            case "local_mean":
                return strictNative ? new org.boofcpp.threshold.NativeLocalMean(width, scale, down, type) :
                        FactoryThresholdBinary.localMean(width, scale, down, type);
            case "block_mean":
                return strictNative ? new org.boofcpp.threshold.NativeBlockMean(width, scale, down, local, type) :
                        FactoryThresholdBinary.blockMean(width, scale, down, local, type);
            case "block_otsu":
                return strictNative ? new org.boofcpp.threshold.NativeBlockOtsu(true, width, 0, scale, down, local,
                        type) : FactoryThresholdBinary.blockOtsu(width, scale, down, local, true, 0, type);
            case "block_minmax":
                return strictNative ? new org.boofcpp.threshold.NativeBlockMinMax(width, scale, down, 1, local, type) :
                        FactoryThresholdBinary.blockMinMax(width, scale, down, local, 1, type);
            default:
                throw new IllegalArgumentException(op);
        }
    }

    @Benchmark public void threshold_fast() { fast.process(input, output); }
    @Benchmark public void threshold_strict() { strict.process(input, output); }
    @Benchmark public void threshold_java() { java.process(input, output); }
}
