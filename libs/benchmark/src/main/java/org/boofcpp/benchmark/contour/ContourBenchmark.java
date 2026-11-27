package org.boofcpp.benchmark.contour;

import boofcv.abst.filter.binary.BinaryLabelContourFinder;
import boofcv.alg.filter.binary.ContourPacked;
import boofcv.factory.filter.binary.BOverrideFactoryBinaryContourFinder;
import boofcv.factory.filter.binary.FactoryBinaryContourFinder;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayU8;
import georegression.struct.point.Point2D_I32;
import org.boofcpp.benchmark.Hook;
import org.boofcpp.benchmark.Images;
import org.boofcpp.benchmark.ImplementationBenchmark;
import org.ddogleg.struct.DogArray;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

import java.util.List;

/// Chang 2004 contours with BoofCV's FactoryBinaryContourFinder of the test image thresholded with global Otsu.
/// chang_load also loads every external contour
@Fork(value = 2)
@State(Scope.Benchmark)
public class ContourBenchmark extends ImplementationBenchmark {
    GrayU8 binary;
    GrayS32 labeled;
    BinaryLabelContourFinder alg;
    DogArray<Point2D_I32> contour = new DogArray<>(Point2D_I32::new);

    @Setup public void setup( BenchmarkParams params ) {
        setupImplementation(params);
        var input = (GrayU8)Images.testImage("U8");
        binary = input.createSameShape();
        FactoryThresholdBinary.globalOtsu(0, 255, 1.0, true, GrayU8.class).process(input, binary);
        labeled = binary.createSameShape(GrayS32.class);
        alg = checked(FactoryBinaryContourFinder.linearChang2004());
    }

    @Benchmark @Hook(type = BOverrideFactoryBinaryContourFinder.class, field = "chang2004")
    public void chang_process() { alg.process(binary, labeled); }

    @Benchmark @Hook(type = BOverrideFactoryBinaryContourFinder.class, field = "chang2004")
    public void chang_load() {
        alg.process(binary, labeled);
        List<ContourPacked> list = alg.getContours();
        for (int i = 0; i < list.size(); i++) {
            contour.reset();
            alg.loadContour(list.get(i).externalIndex, contour);
        }
    }
}
