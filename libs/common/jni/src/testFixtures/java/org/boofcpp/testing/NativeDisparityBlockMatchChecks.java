package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.abst.disparity.StereoDisparity;
import boofcv.abst.disparity.StereoDisparityBlockMatch;
import boofcv.alg.misc.ImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.factory.transform.census.CensusVariants;
import boofcv.struct.border.BorderType;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

import static org.junit.jupiter.api.Assertions.*;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

/// Checks of a variant's stereo block matching against BoofCV's Java implementation, shared by every variant. Results
/// must be identical. A variant's test class extends this and passes its hook and the input image types it supports.
/// The inputs are GrayU8 and GrayF32; comparisons with a type the variant doesn't support are skipped
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeDisparityBlockMatchChecks extends BoofCPPStandardJUnit {
    private final BOverrideStereoDisparity.CreateBlockMatch hook;
    private final Class[] imageTypes;

    protected NativeDisparityBlockMatchChecks( BOverrideStereoDisparity.CreateBlockMatch hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    boolean supports( Class type ) {
        for (Class c : imageTypes) {
            if (c == type)
                return true;
        }
        return false;
    }

    /// The variant's block matcher. It must support the configuration
    <T extends ImageGray<T>, DI extends ImageGray<DI>>
    StereoDisparityBlockMatch<T, DI> create( ConfigDisparityBM config, Class<T> imageType, Class<DI> dispType ) {
        StereoDisparityBlockMatch<T, DI> alg = hook.create(config, imageType, dispType);
        assertNotNull(alg, "The variant doesn't support " + imageType.getSimpleName());
        return alg;
    }

    @AfterEach void after() {
        BOverrideStereoDisparity.opCreateBlockMatch = null;
    }

    /// Right image is the left shifted by 4 plus noise, so there are real matches
    GrayU8[] createPair( int width, int height ) {
        return createPair(width, height, 255);
    }

    /// A small maxValue creates low texture with many equal scores, which exercises ties and the texture check
    GrayU8[] createPair( int width, int height, int maxValue ) {
        var left = new GrayU8(width, height);
        ImageMiscOps.fillUniform(left, rand, 0, maxValue);
        var right = left.createSameShape();
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int noise = maxValue < 255 ? rand.nextInt(3) - 1 : rand.nextInt(21) - 10;
                int v = left.get(Math.min(width - 1, x + 4), y) + noise;
                right.set(x, y, Math.max(0, Math.min(maxValue, v)));
            }
        }
        return new GrayU8[]{left, right};
    }

    /// Same as [#createPair(int,int,int)] as GrayF32, plus a fractional part so that floating point rounding matters
    GrayF32[] createPairF32( int width, int height, int maxValue ) {
        GrayU8[] pair = createPair(width, height, maxValue);
        var ret = new GrayF32[2];
        for (int i = 0; i < 2; i++) {
            ret[i] = new GrayF32(width, height);
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    ret[i].set(x, y, pair[i].get(x, y) + rand.nextFloat()*0.5f);
                }
            }
        }
        return ret;
    }

    /// Every error type: SAD, SSD, ZSSD, NCC, then each census variant
    static List<Consumer<ConfigDisparityBM>> errorTypes() {
        var list = new ArrayList<Consumer<ConfigDisparityBM>>();
        for (DisparityError error : new DisparityError[]{
                DisparityError.SAD, DisparityError.SSD, DisparityError.ZSSD, DisparityError.NCC}) {
            list.add(c -> c.errorType = error);
        }
        for (CensusVariants variant : CensusVariants.values()) {
            list.add(c -> {
                c.errorType = DisparityError.CENSUS;
                c.configCensus.variant = variant;
            });
        }
        return list;
    }

    static ConfigDisparityBM createConfig( boolean subpixel ) {
        var config = new ConfigDisparityBM();
        config.errorType = DisparityError.SAD;
        config.disparityRange = 20;
        config.subpixel = subpixel;
        return config;
    }

    /// Java and native with the same configuration must produce identical disparity and score
    ///
    /// @return number of pixels with a valid disparity
    <T extends ImageGray<T>, DI extends ImageGray<DI>>
    int compare( ConfigDisparityBM config, Class<DI> dispType, T left, T right ) {
        Class<T> imageType = (Class<T>)left.getClass();
        if (!supports(imageType))
            return 0;
        BOverrideStereoDisparity.opCreateBlockMatch = null;
        StereoDisparity<T, DI> java = FactoryStereoDisparity.blockMatch(config, imageType, dispType);
        StereoDisparity<T, DI> found = create(config, imageType, dispType);

        java.process(left, right);
        found.process(left, right);

        assertMatches(java.getDisparity(), found.getDisparity());
        if (config.saveScore) {
            assertMatches(java.getDisparityScore(), found.getDisparityScore());
        } else {
            assertNull(found.getDisparityScore());
        }
        assertEquals(java.getDisparityMin(), found.getDisparityMin());
        assertEquals(java.getDisparityRange(), found.getDisparityRange());
        assertEquals(java.getInvalidValue(), found.getInvalidValue());
        assertEquals(java.getBorderX(), found.getBorderX());
        assertEquals(java.getBorderY(), found.getBorderY());
        assertEquals(java.getDisparityType(), found.getDisparityType());

        int valid = 0;
        for (int y = 0; y < left.height; y++) {
            for (int x = 0; x < left.width; x++) {
                if (GeneralizedImageOps.get(found.getDisparity(), x, y) < found.getInvalidValue())
                    valid++;
            }
        }
        return valid;
    }

    /// Every combination of the settings that change the result, with every error type
    @Test void compareGrid() {
        // odd size, and narrower than the largest disparity range
        compareGrid(createPair(37, 29), createPair(37, 29, 4));
    }

    /// Same as [#compareGrid()] with floating point images
    @Test void compareGridF32() {
        compareGrid(createPairF32(37, 29, 255), createPairF32(37, 29, 4));
    }

    <T extends ImageGray<T>> void compareGrid( T[] textured, T[] lowTexture ) {
        assumeTrue(supports(textured[0].getClass()));
        int[][] radii = {{1, 1}, {3, 2}, {2, 4}};
        // {min, range}: positive, positive offset, negative, only negative, range larger than the image
        int[][] disparities = {{0, 20}, {5, 10}, {-6, 15}, {-12, 8}, {0, 60}};
        // {maxPerPixelError, validateRtoL, texture}: defaults, all off, then each check by itself
        double[][] checks = {{0, 1, 0.15}, {-1, -1, 0}, {20, -1, 0}, {-1, 0, 0}, {-1, -1, 0.3}};
        BorderType[] borders = {BorderType.EXTENDED, BorderType.REFLECT, BorderType.WRAP, BorderType.ZERO};
        List<Consumer<ConfigDisparityBM>> errors = errorTypes();

        int count = 0;
        int withValid = 0;
        for (Consumer<ConfigDisparityBM> error : errors)
        for (int[] radius : radii)
            for (int[] disparity : disparities)
                for (double[] check : checks)
                    for (BorderType border : borders)
                        for (boolean subpixel : new boolean[]{false, true}) {
                            ConfigDisparityBM config = createConfig(subpixel);
                            error.accept(config);
                            config.regionRadiusX = radius[0];
                            config.regionRadiusY = radius[1];
                            config.disparityMin = disparity[0];
                            config.disparityRange = disparity[1];
                            // The per pixel error is in different units
                            config.maxPerPixelError = check[0];
                            if (check[0] > 0 && config.errorType == DisparityError.CENSUS)
                                config.maxPerPixelError = 3;
                            else if (check[0] > 0 && config.errorType.isSquared())
                                config.maxPerPixelError = check[0]*check[0];
                            config.validateRtoL = (int)check[1];
                            config.texture = check[2];
                            config.border = border;
                            for (T[] pair : List.of(textured, lowTexture)) {
                                int valid = subpixel ?
                                        compare(config, GrayF32.class, pair[0], pair[1]) :
                                        compare(config, GrayU8.class, pair[0], pair[1]);
                                if (valid > 0)
                                    withValid++;
                                count++;
                            }
                        }
        assertEquals(errors.size()*3*5*5*4*2*2, count);
        // Make sure the comparisons aren't trivial because everything is invalid
        assertTrue(withValid > count*0.9, "withValid=" + withValid);
    }

    /// The floating point running sum is recomputed every catastrophicReset rows
    @Test void catastrophicReset() {
        GrayF32[] pair = createPairF32(40, 45, 255);
        GrayU8[] pairU8 = createPair(40, 45);
        for (DisparityError error : new DisparityError[]{DisparityError.SAD, DisparityError.ZSSD, DisparityError.NCC}) {
            for (int reset : new int[]{1, 2, 7, 25, 1000}) {
                ConfigDisparityBM config = createConfig(true);
                config.errorType = error;
                config.catastrophicReset = reset;
                compare(config, GrayF32.class, pair[0], pair[1]);
                if (error != DisparityError.SAD)
                    compare(config, GrayF32.class, pairU8[0], pairU8[1]);
            }
        }
    }

    /// NCC with and without normalizing the input, and different eps
    @Test void nccOptions() {
        GrayU8[] pairU8 = createPair(40, 30);
        GrayF32[] pairF32 = createPairF32(40, 30, 255);
        for (boolean normalize : new boolean[]{true, false}) {
            for (double eps : new double[]{5.5e-6, 1e-3, 0.0}) {
                ConfigDisparityBM config = createConfig(true);
                config.errorType = DisparityError.NCC;
                config.configNCC.normalizeInput = normalize;
                config.configNCC.eps = eps;
                compare(config, GrayF32.class, pairU8[0], pairU8[1]);
                compare(config, GrayF32.class, pairF32[0], pairF32[1]);
            }
        }
    }

    @Test void compareDefaults() {
        assumeTrue(supports(GrayU8.class));
        GrayU8[] pair = createPair(60, 45);
        compare(createConfig(true), GrayF32.class, pair[0], pair[1]);
        compare(createConfig(false), GrayU8.class, pair[0], pair[1]);
        // BoofCV's default configuration is census
        compare(new ConfigDisparityBM(), GrayF32.class, pair[0], pair[1]);
    }

    /// The configuration's border is also used by the census transform and the local statistics of ZSSD and NCC
    @Test void border() {
        GrayU8[] pair = createPair(40, 30);
        GrayF32[] pairF32 = createPairF32(40, 30, 255);
        for (Consumer<ConfigDisparityBM> error : errorTypes()) {
            var config = new ConfigDisparityBM();
            error.accept(config);
            for (BorderType border : new BorderType[]{BorderType.ZERO, BorderType.EXTENDED, BorderType.WRAP}) {
                config.border = border;
                compare(config, GrayF32.class, pair[0], pair[1]);
                compare(config, GrayF32.class, pairF32[0], pairF32[1]);
            }
        }
    }

    /// Images smaller than the census region and the block. Every pixel is in the border
    @Test void smallerThanRegion() {
        assumeTrue(supports(GrayU8.class));
        int javaFailed = 0;
        for (int[] shape : new int[][]{{1, 1}, {1, 4}, {4, 1}, {2, 3}, {3, 2}, {5, 4}}) {
            GrayU8[] pair = createPair(shape[0], shape[1]);
            for (Consumer<ConfigDisparityBM> error : errorTypes()) {
                var config = new ConfigDisparityBM();
                config.disparityRange = 3;
                error.accept(config);
                for (BorderType border : new BorderType[]{BorderType.REFLECT, BorderType.WRAP, BorderType.EXTENDED}) {
                    config.border = border;
                    boolean statistics = config.errorType == DisparityError.ZSSD ||
                            config.errorType == DisparityError.NCC;
                    if (statistics && (shape[0] < config.regionRadiusX || shape[1] < config.regionRadiusY)) {
                        // BoofCV's mean filter with a border writes outside the image when it's smaller than the
                        // radius. Native processes it
                        BOverrideStereoDisparity.opCreateBlockMatch = null;
                        var java = FactoryStereoDisparity.blockMatch(config, GrayU8.class, GrayF32.class);
                        assertThrows(ArrayIndexOutOfBoundsException.class, () -> java.process(pair[0], pair[1]));
                        create(config, GrayU8.class, GrayF32.class).process(pair[0], pair[1]);
                        javaFailed++;
                        continue;
                    }
                    compare(config, GrayF32.class, pair[0], pair[1]);
                }
            }
        }
        // 1x1, 1x4, 4x1, 2x3 and 3x2 with ZSSD and NCC and three borders
        assertEquals(5*2*3, javaFailed);
    }

    /// Every error type with a supported image type gives the variant's block matcher. Other types give null or throw a
    /// RuntimeException, so BoofCV uses its Java implementation
    @Test void supportedTypes() {
        var config = new ConfigDisparityBM();
        for (DisparityError error : DisparityError.values()) {
            config.errorType = error;
            for (Class type : imageTypes) {
                assertNotNull(hook.create(config, type, GrayF32.class), error.name() + " " + type.getSimpleName());
            }
            for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
                try {
                    assertNull(hook.create(config, type, GrayF32.class), error.name() + " " + type.getSimpleName());
                } catch (RuntimeException ignore) {}
            }
        }
    }

    @Test void saveScoreOff() {
        assumeTrue(supports(GrayU8.class));
        GrayU8[] pair = createPair(60, 45);
        ConfigDisparityBM config = createConfig(true);
        config.saveScore = false;
        compare(config, GrayF32.class, pair[0], pair[1]);
    }

    /// Every check off. BoofCV then uses a selector that doesn't write the score
    @Test void everyCheckOff() {
        assumeTrue(supports(GrayU8.class));
        GrayU8[] pair = createPair(60, 45);
        ConfigDisparityBM config = createConfig(false);
        config.maxPerPixelError = -1;
        config.validateRtoL = -1;
        config.texture = 0;
        compare(config, GrayU8.class, pair[0], pair[1]);
    }

    @Test void subImages() {
        GrayU8[] pair = createPair(60, 45);
        GrayF32[] pairF32 = createPairF32(60, 45, 255);
        for (Consumer<ConfigDisparityBM> error : errorTypes()) {
            ConfigDisparityBM config = createConfig(true);
            error.accept(config);
            compare(config, GrayF32.class,
                    BoofTesting.createSubImageOf(pair[0]), BoofTesting.createSubImageOf(pair[1]));
            compare(config, GrayF32.class,
                    BoofTesting.createSubImageOf(pairF32[0]), BoofTesting.createSubImageOf(pairF32[1]));
        }
    }

    /// Process images of different sizes with the same instance
    @Test void changeImageSize() {
        assumeTrue(supports(GrayU8.class));
        ConfigDisparityBM config = createConfig(true);
        BOverrideStereoDisparity.opCreateBlockMatch = null;
        StereoDisparity<GrayU8, GrayF32> java = FactoryStereoDisparity.blockMatch(config, GrayU8.class, GrayF32.class);
        StereoDisparity<GrayU8, GrayF32> found = create(config, GrayU8.class, GrayF32.class);
        for (int[] shape : new int[][]{{60, 45}, {30, 20}, {70, 50}}) {
            GrayU8[] pair = createPair(shape[0], shape[1]);
            java.process(pair[0], pair[1]);
            found.process(pair[0], pair[1]);
            assertMatches(java.getDisparity(), found.getDisparity());
            assertMatches(java.getDisparityScore(), found.getDisparityScore());
        }
    }

    /// With the hook registered, BoofCV's factory returns the variant's block matcher if it supports the
    /// configuration, and BoofCV's otherwise
    @Test void factoryUsesHook() {
        BOverrideStereoDisparity.opCreateBlockMatch = hook;
        ConfigDisparityBM config = createConfig(true);
        for (Consumer<ConfigDisparityBM> error : errorTypes()) {
            error.accept(config);
            for (Class type : imageTypes) {
                assertEquals(create(config, type, GrayF32.class).getClass(),
                        FactoryStereoDisparity.blockMatch(config, type, GrayF32.class).getClass());
            }
        }

        config.errorType = DisparityError.SAD;
        if (!supports(GrayU16.class)) {
            BOverrideStereoDisparity.opCreateBlockMatch = null;
            Class java = FactoryStereoDisparity.blockMatch(config, GrayU16.class, GrayF32.class).getClass();
            BOverrideStereoDisparity.opCreateBlockMatch = hook;
            assertEquals(java, FactoryStereoDisparity.blockMatch(config, GrayU16.class, GrayF32.class).getClass());
        }
    }

    /// Errors in C++ must become Java exceptions and not crash the JVM
    @Test void exceptionsBecomeJava() {
        assumeTrue(supports(GrayU8.class));
        // range too large for a U8 disparity image is detected while processing, same as Java
        ConfigDisparityBM config = createConfig(false);
        config.disparityRange = 255;
        var found = create(config, GrayU8.class, GrayU8.class);
        GrayU8[] pair = createPair(30, 20);
        assertThrows(IllegalArgumentException.class, () -> found.process(pair[0], pair[1]));

        // and it can still be used afterwards with valid input
        config.disparityRange = 20;
        var ok = create(config, GrayU8.class, GrayU8.class);
        ok.process(pair[0], pair[1]);
    }
}
