package org.boofcpp.disparity;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BenchmarkUtils;
import org.boofcpp.BoofCPP;
import org.boofcpp.contour.BenchmarkContour;
import org.openjdk.jmh.annotations.*;

import java.awt.image.BufferedImage;
import java.util.Random;
import java.util.concurrent.TimeUnit;

/// Stereo block matching. The stereo pair is synthetic: the test image downsampled to 672x504, the right image is
/// the left shifted by 16 pixels plus noise.
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@State(Scope.Benchmark)
@Fork(value = 1)
public class BenchmarkDisparityBlockMatch {
    public static final int SHIFT = 16;

    GrayU8 left, right;

    StereoDisparity<GrayU8, GrayF32> sad_java;
    StereoDisparity<GrayU8, GrayF32> sad_native;
    StereoDisparity<GrayU8, GrayF32> census_java;
    StereoDisparity<GrayU8, GrayF32> census_native;

    static {
        BoofCPP.loadlib();
        // The speed target is against single-threaded Java
        boofcv.concurrency.BoofConcurrency.USE_CONCURRENT = false;
    }

    public BenchmarkDisparityBlockMatch() {
        BufferedImage buffered = BenchmarkUtils.loadImage(BenchmarkContour.IMAGE_PATH);
        GrayU8 full = ConvertBufferedImage.convertFrom(buffered, (GrayU8)null);
        left = new GrayU8(full.width/6, full.height/6);
        AverageDownSampleOps.down(full, 6, left);
        right = createRight(left, SHIFT, 0xBEEF);

        sad_java = FactoryStereoDisparity.blockMatch(createConfig(DisparityError.SAD), GrayU8.class, GrayF32.class);
        sad_native = new NativeDisparityBlockMatch<>(createConfig(DisparityError.SAD), GrayU8.class, GrayF32.class);
        census_java = FactoryStereoDisparity.blockMatch(createConfig(DisparityError.CENSUS), GrayU8.class, GrayF32.class);
        census_native = new NativeDisparityBlockMatch<>(createConfig(DisparityError.CENSUS), GrayU8.class, GrayF32.class);
    }

    /// Right image is the left shifted by 'shift' pixels plus uniform noise in `[-3,3]`
    public static GrayU8 createRight( GrayU8 left, int shift, long seed ) {
        var rand = new Random(seed);
        GrayU8 right = left.createSameShape();
        for (int y = 0; y < left.height; y++) {
            for (int x = 0; x < left.width; x++) {
                int v = left.get(Math.min(left.width - 1, x + shift), y) + rand.nextInt(7) - 3;
                right.set(x, y, Math.max(0, Math.min(255, v)));
            }
        }
        return right;
    }

    /// Default configuration, which has subpixel and all checks on
    public static ConfigDisparityBM createConfig( DisparityError error ) {
        var config = new ConfigDisparityBM();
        config.errorType = error;
        return config;
    }

    @Benchmark public void sad_java() {sad_java.process(left, right);}

    @Benchmark public void sad_native() {sad_native.process(left, right);}

    @Benchmark public void census_java() {census_java.process(left, right);}

    @Benchmark public void census_native() {census_native.process(left, right);}
}
