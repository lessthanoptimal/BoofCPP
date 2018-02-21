package org.boofcpp.contour;

import boofcv.abst.filter.binary.BinaryContourFinder;
import boofcv.abst.filter.binary.BinaryContourFinderChang2004;
import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.filter.binary.ContourPacked;
import boofcv.alg.filter.binary.ThresholdImageOps;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.io.image.UtilImageIO;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayU8;
import georegression.struct.point.Point2D_I32;
import org.boofcpp.BoofCPP;
import org.boofcpp.threshold.*;
import org.ddogleg.struct.FastQueue;
import org.openjdk.jmh.annotations.*;

import java.awt.image.BufferedImage;
import java.net.URL;
import java.util.List;
import java.util.concurrent.TimeUnit;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value=2)
public class BenchmarkContour {

    private static final String IMAGE_PATH="/org/boofcpp/threshold/test_image.jpg";


    GrayU8 binary;
    GrayS32 labeled;

    BinaryContourFinder algNative = new NativeChang2004();
    BinaryContourFinder algJava = new BinaryContourFinderChang2004();



    static {
        BoofCPP.loadlib();
    }
    public BenchmarkContour() {

        URL url = getClass().getResource(IMAGE_PATH);
        if( url == null )
            throw new RuntimeException("Can't find resource! "+IMAGE_PATH);
//        System.out.println("URL path "+url.getFile());
        BufferedImage buffered = UtilImageIO.loadImage(url.getFile());
        if( buffered == null )
            throw new RuntimeException("Couldn't load the image");
        GrayU8 input=ConvertBufferedImage.convertFrom(buffered,(GrayU8)null);

        binary = input.createSameShape();
        FactoryThresholdBinary.globalOtsu(0,255,true,GrayU8.class)
                .process(input,binary);

        labeled = binary.createSameShape(GrayS32.class);
    }

    @Benchmark
    public void chang_native() { algNative.process(binary,labeled); }

    @Benchmark
    public void chang_java() { algJava.process(binary,labeled); }

    @Benchmark
    public void chang_load_native() { process_load(algNative); }

    @Benchmark
    public void chang_load_java() { process_load(algJava); }

    public void process_load(BinaryContourFinder alg) {
        alg.process(binary,labeled);
        FastQueue<Point2D_I32> queue = new FastQueue<>(Point2D_I32.class,true);

        List<ContourPacked> list = algNative.getContours();
        for (int i = 0; i < list.size(); i++) {
            queue.reset();
            algNative.loadContour(list.get(i).externalIndex,queue);
        }
    }

}
