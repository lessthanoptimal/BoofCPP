package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.io.image.UtilImageIO;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.openjdk.jmh.annotations.*;

import java.awt.image.BufferedImage;
import java.net.URL;
import java.util.concurrent.TimeUnit;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value=1)
public class BenchmarkThresholding {

    private static final String IMAGE_PATH="/org/boofcpp/threshold/test_image.jpg";

    Class<GrayU8> type = GrayU8.class;
    GrayU8 input;
    GrayU8 output;

    boolean down = true;
    double scale = 0.95;
    ConfigLength regionWidth = ConfigLength.fixed(40);
    boolean local_blocks = false;


    InputToBinary<GrayU8> global_fixed_native = new NativeGlobalFixed<>(125,down, type);
    InputToBinary<GrayU8> global_fixed_java = FactoryThresholdBinary.globalFixed(125,down, type);

    InputToBinary<GrayU8> global_otsu_native = new NativeGlobalOtsu<>(0,255,down, type);
    InputToBinary<GrayU8> global_otsu_java = FactoryThresholdBinary.globalOtsu(0,255,down, type);

    InputToBinary<GrayU8> local_mean_native = new NativeLocalMean<>(regionWidth, scale, down, type);
    InputToBinary<GrayU8> local_mean_java = FactoryThresholdBinary.localMean(regionWidth, scale, down, type);

    InputToBinary<GrayU8> block_minmax_native = new NativeBlockMinMax<>(regionWidth, scale, down, 1, local_blocks,type);
    InputToBinary<GrayU8> block_minmax_java = FactoryThresholdBinary.blockMinMax( regionWidth, scale, down, 1, local_blocks,type);
    InputToBinary<GrayU8> block_mean_native = new NativeBlockMean<>(regionWidth, scale, down, local_blocks, type);
    InputToBinary<GrayU8> block_mean_java = FactoryThresholdBinary.blockMean( regionWidth, scale, down, local_blocks, type);
    InputToBinary<GrayU8> block_otsu_native = new NativeBlockOtsu<>(true, regionWidth, 0, scale, down, local_blocks, type);
    InputToBinary<GrayU8> block_otsu_java = FactoryThresholdBinary.blockOtsu(true, regionWidth, 0, scale, down, local_blocks, type);

    static {
        BoofCPP.loadlib();
    }
    public BenchmarkThresholding () {

        URL url = getClass().getResource(IMAGE_PATH);
        if( url == null )
            throw new RuntimeException("Can't find resource! "+IMAGE_PATH);
//        System.out.println("URL path "+url.getFile());
        BufferedImage buffered = UtilImageIO.loadImage(url.getFile());
        if( buffered == null )
            throw new RuntimeException("Couldn't load the image");
        input=ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);

        output = new GrayU8(input.width,input.height);
    }

    @Benchmark
    public void global_fixed_native() { global_fixed_native.process(input,output); }

    @Benchmark
    public void global_fixed_java() { global_fixed_java.process(input,output); }

    @Benchmark
    public void global_otsu_native() { global_otsu_native.process(input,output); }

    @Benchmark
    public void global_otsu_java() { global_otsu_java.process(input,output); }

    @Benchmark
    public void local_mean_native() { local_mean_native.process(input,output); }

    @Benchmark
    public void local_mean_java() { local_mean_java.process(input,output); }

    @Benchmark
    public void block_minmax_native() { block_minmax_native.process(input,output); }

    @Benchmark
    public void block_minmax_java() {  block_minmax_java.process(input,output); }

    @Benchmark
    public void block_mean_native() { block_mean_native.process(input,output); }

    @Benchmark
    public void block_mean_java() { block_mean_java.process(input,output); }

    @Benchmark
    public void block_otsu_native() { block_otsu_native.process(input,output); }

    @Benchmark
    public void block_otsu_java() { block_otsu_java.process(input,output); }
}
