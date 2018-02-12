package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.io.image.UtilImageIO;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.runner.RunnerException;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.concurrent.TimeUnit;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value=2)
public class BenchmarkThresholding {

    private static final String IMAGE_PATH="/org/boofcpp/threshold/image_fremont_040.jpg";

    GrayU8 input;
    GrayU8 output;

    ConfigLength regionWidth = ConfigLength.fixed(40);
    Class<GrayU8> type = GrayU8.class;
    boolean down = true;

    InputToBinary<GrayU8> block_otsu_native = new NativeBlockOtsu<>(true, regionWidth, 0, 0.95, down, false, type);
    InputToBinary<GrayU8> block_otsu_java = FactoryThresholdBinary.blockOtsu(true, regionWidth, 0, 0.95, down, false, type);

    InputToBinary<GrayU8> local_mean_native = new NativeLocalMean<>(regionWidth, 0.95, down, type);
    InputToBinary<GrayU8> local_mean_java = FactoryThresholdBinary.localMean(regionWidth, 0.95, down, type);

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
    public void block_otsu_java() {
        block_otsu_java.process(input,output);
    }

    @Benchmark
    public void block_otsu_native() {
        block_otsu_native.process(input,output);
    }

    @Benchmark
    public void local_mean_native() {
        local_mean_native.process(input,output);
    }

    @Benchmark
    public void local_mean_java() {
        local_mean_java.process(input,output);
    }
}
