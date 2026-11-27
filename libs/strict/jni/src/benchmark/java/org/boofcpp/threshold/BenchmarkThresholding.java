package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.concurrency.BoofConcurrency;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.core.image.ConvertImage;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.testing.BenchmarkUtils;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.NativeConcurrency;
import org.openjdk.jmh.annotations.*;

import java.awt.image.BufferedImage;
import java.util.concurrent.TimeUnit;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value=1)
public class BenchmarkThresholding
{
    Class<GrayU8> type = GrayU8.class;
    GrayU8 input;
    GrayF32 inputF32;
    GrayU8 output;

    boolean down = true;
    double scale = 0.95;
    ConfigLength regionWidth = ConfigLength.fixed(40);
    boolean local_blocks = false;

    /// Threads of native and of BoofCV's Java code. 1 is single threaded, which is the speed target against Java
    @Param({"1", "4"})
    public int threads;

    InputToBinary<GrayU8> global_fixed_native;
    InputToBinary<GrayU8> global_fixed_java;
    InputToBinary<GrayU8> global_otsu_native;
    InputToBinary<GrayU8> global_otsu_java;
    InputToBinary<GrayF32> global_otsu_native_F32;
    InputToBinary<GrayF32> global_otsu_java_F32;
    InputToBinary<GrayU8> local_mean_native;
    InputToBinary<GrayU8> local_mean_java;
    InputToBinary<GrayU8> block_minmax_native;
    InputToBinary<GrayU8> block_minmax_java;
    InputToBinary<GrayF32> block_minmax_native_F32;
    InputToBinary<GrayF32> block_minmax_java_F32;
    InputToBinary<GrayU8> block_mean_native;
    InputToBinary<GrayU8> block_mean_java;
    InputToBinary<GrayU8> block_otsu_native;
    InputToBinary<GrayU8> block_otsu_java;

    static {
        BoofCPPStrict.loadlib();
    }

    @Setup public void setup() {
        NativeConcurrency.setMaxThreads(threads);
        BoofConcurrency.USE_CONCURRENT = threads > 1;
        BoofConcurrency.setMaxThreads(threads);

        // BoofCV's factories pick concurrent implementations when the filters are created
        global_fixed_native = new NativeGlobalFixed<>(125,down, type);
        global_fixed_java = FactoryThresholdBinary.globalFixed(125,down, type);
        global_otsu_native = new NativeGlobalOtsu<>(0,255,1.0,down, type);
        global_otsu_java = FactoryThresholdBinary.globalOtsu(0,255,1.0,down, type);
        global_otsu_native_F32 = new NativeGlobalOtsu<>(0,255,1.0,down, GrayF32.class);
        global_otsu_java_F32 = FactoryThresholdBinary.globalOtsu(0,255,1.0,down, GrayF32.class);
        local_mean_native = new NativeLocalMean<>(regionWidth, scale, down, type);
        local_mean_java = FactoryThresholdBinary.localMean(regionWidth, scale, down, type);
        block_minmax_native = new NativeBlockMinMax<>(regionWidth, scale, down, 1, local_blocks,type);
        block_minmax_java = FactoryThresholdBinary.blockMinMax( regionWidth, scale, down, local_blocks, 1, type);
        block_minmax_native_F32 = new NativeBlockMinMax<>(regionWidth, scale, down, 1, local_blocks, GrayF32.class);
        block_minmax_java_F32 = FactoryThresholdBinary.blockMinMax(regionWidth, scale, down, local_blocks, 1, GrayF32.class);
        block_mean_native = new NativeBlockMean<>(regionWidth, scale, down, local_blocks, type);
        block_mean_java = FactoryThresholdBinary.blockMean( regionWidth, scale, down, local_blocks, type);
        block_otsu_native = new NativeBlockOtsu<>(true, regionWidth, 0, scale, down, local_blocks, type);
        block_otsu_java = FactoryThresholdBinary.blockOtsu(regionWidth, scale, down, local_blocks, true, 0, type);

        BufferedImage buffered = BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE);
        input=ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);
        inputF32 = ConvertImage.convert(input, (GrayF32)null);

        output = new GrayU8(input.width,input.height);
    }

    @Benchmark public void global_fixed_native_U8() { global_fixed_native.process(input,output); }
    @Benchmark public void global_fixed_java_U8() { global_fixed_java.process(input,output); }
    @Benchmark public void global_otsu_native_U8() { global_otsu_native.process(input,output); }
    @Benchmark public void global_otsu_java_U8() { global_otsu_java.process(input,output); }
    @Benchmark public void global_otsu_native_F32() { global_otsu_native_F32.process(inputF32,output); }
    @Benchmark public void global_otsu_java_F32() { global_otsu_java_F32.process(inputF32,output); }
    @Benchmark public void local_mean_native_U8() { local_mean_native.process(input,output); }
    @Benchmark public void local_mean_java_U8() { local_mean_java.process(input,output); }
    @Benchmark public void block_minmax_native_U8() { block_minmax_native.process(input,output); }
    @Benchmark public void block_minmax_java_U8() { block_minmax_java.process(input,output); }
    @Benchmark public void block_minmax_native_F32() { block_minmax_native_F32.process(inputF32,output); }
    @Benchmark public void block_minmax_java_F32() { block_minmax_java_F32.process(inputF32,output); }
    @Benchmark public void block_mean_native_U8() { block_mean_native.process(input,output); }
    @Benchmark public void block_mean_java_U8() { block_mean_java.process(input,output); }
    @Benchmark public void block_otsu_native_U8() { block_otsu_native.process(input,output); }
    @Benchmark public void block_otsu_java_U8() { block_otsu_java.process(input,output); }
}
