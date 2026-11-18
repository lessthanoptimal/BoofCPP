package org.boofcpp.fast.transform;

import boofcv.alg.transform.census.BOverrideCensusTransform;
import boofcv.alg.transform.census.CensusTransform;
import boofcv.alg.transform.census.GCensusTransform;
import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import georegression.struct.point.Point2D_I32;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.BenchmarkUtils;
import org.ddogleg.struct.DogArray;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

/// Census transforms of the test image with a reflected border: fast through BoofCV, strict's native code and
/// BoofCV's Java code. sample samples a 7x7 block, BoofCV's BLOCK_7_7
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
@SuppressWarnings({"rawtypes", "unchecked"})
public class BenchmarkCensusTransform {
    @Param({"U8", "U16", "F32"})
    public String image;

    ImageGray input;
    ImageBorder border;
    DogArray<Point2D_I32> block = CensusTransform.createBlockSamples(3);
    GrayU8 dense3x3 = new GrayU8(1, 1);
    GrayS32 dense5x5 = new GrayS32(1, 1);
    GrayS64 out64 = new GrayS64(1, 1);

    /// The census hook BoofCPP.initialize() registered
    BOverrideCensusTransform.Census registered;
    org.boofcpp.transform.NativeCensusTransform strict = new org.boofcpp.transform.NativeCensusTransform();

    static {
        BoofCPP.loadlib();
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    @Setup public void setup() {
        BoofCPP.initialize();
        registered = BOverrideCensusTransform.census;

        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        Class type = image.equals("U8") ? GrayU8.class : image.equals("U16") ? GrayU16.class : GrayF32.class;
        input = GeneralizedImageOps.createSingleBand(type, gray.width, gray.height);
        GConvertImage.convert(gray, input);
        border = FactoryImageBorder.single(BorderType.REFLECT, type);
        dense3x3.reshape(gray.width, gray.height);
        dense5x5.reshape(gray.width, gray.height);
        out64.reshape(gray.width, gray.height);
    }

    @Benchmark public void dense3x3_fast() { hooked(registered); GCensusTransform.dense3x3(input, dense3x3, border); }
    @Benchmark public void dense3x3_strict() { strict.dense3x3(input, dense3x3, border); }
    @Benchmark public void dense3x3_java() { hooked(null); GCensusTransform.dense3x3(input, dense3x3, border); }
    @Benchmark public void dense5x5_fast() { hooked(registered); GCensusTransform.dense5x5(input, dense5x5, border); }
    @Benchmark public void dense5x5_strict() { strict.dense5x5(input, dense5x5, border); }
    @Benchmark public void dense5x5_java() { hooked(null); GCensusTransform.dense5x5(input, dense5x5, border); }
    @Benchmark public void sample_fast() {
        hooked(registered);
        GCensusTransform.sample_S64(input, block, out64, border, null);
    }

    @Benchmark public void sample_strict() { strict.sample_S64(input, block, out64, border); }

    @Benchmark public void sample_java() {
        hooked(null);
        GCensusTransform.sample_S64(input, block, out64, border, null);
    }

    private static void hooked( BOverrideCensusTransform.Census census ) {
        BOverrideCensusTransform.census = census;
    }
}
