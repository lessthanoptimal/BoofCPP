package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.alg.transform.census.BOverrideCensusTransform;
import boofcv.alg.transform.census.CensusTransform;
import boofcv.alg.transform.census.GCensusTransform;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.BorderIndex1D_Extend;
import boofcv.core.image.border.BorderIndex1D_Wrap;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.border.ImageBorder1D_F32;
import boofcv.struct.border.ImageBorder1D_S32;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageBase;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.InterleavedU16;
import georegression.struct.point.Point2D_I32;
import org.ddogleg.struct.DogArray;
import org.ddogleg.struct.FastAccess;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.function.Supplier;

import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's census transform hook against BoofCV's Java code, shared by every variant. Census images
/// are integers, so results must be identical. A variant's test class extends this and passes its hook and the input
/// image types it supports
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeCensusChecks extends BoofCPPStandardJUnit {
    /// Shapes larger than every sampled region, smaller than all of them, and in between
    private static final int[][] SHAPES = {{61, 45}, {9, 6}, {3, 2}, {1, 1}};

    private final BOverrideCensusTransform.Census hook;
    private final Class[] imageTypes;

    protected NativeCensusChecks( BOverrideCensusTransform.Census hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    @Test
    public void compareDense3x3() {
        compare(( census, input, border ) -> {
            var output = new GrayU8(input.width, input.height);
            census.dense3x3(input, output, border);
            return output;
        });
    }

    @Test
    public void compareDense5x5() {
        compare(( census, input, border ) -> {
            var output = new GrayS32(input.width, input.height);
            census.dense5x5(input, output, border);
            return output;
        });
    }

    /// The sampled regions BoofCV's CensusVariants use, and a sparse one with points at different distances
    @Test
    public void compareSampleS64() {
        for (DogArray<Point2D_I32> samples : sampleSets()) {
            compare(( census, input, border ) -> {
                var output = new GrayS64(input.width, input.height);
                census.sample_S64(input, samples, output, border);
                return output;
            });
        }
    }

    /// Input types, borders and samples the variant doesn't support throw a RuntimeException. Through BoofCV the
    /// results are then BoofCV's own
    @Test
    public void unsupportedFallsBackToJava() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 10, 8);
            DogArray<Point2D_I32> samples = CensusTransform.createBlockSamples(1);
            assertThrows(RuntimeException.class, () -> hook.dense3x3(input, new GrayU8(10, 8), null));
            assertThrows(RuntimeException.class, () -> hook.dense5x5(input, new GrayS32(10, 8), null));
            assertThrows(RuntimeException.class, () -> hook.sample_S64(input, samples, new GrayS64(10, 8), null));
        }

        for (Class type : imageTypes) {
            ImageGray input = randomImage(type, 20, 15);
            List<ImageBorder> borders = new ArrayList<>();
            borders.add(type == GrayF32.class ?
                    new ImageBorder1D_F32(new BorderIndex1D_Extend(), new BorderIndex1D_Wrap()) :
                    new ImageBorder1D_S32(new BorderIndex1D_Extend(), new BorderIndex1D_Wrap()));
            if (type != GrayF32.class)
                borders.add(FactoryImageBorder.singleValue(-1, type));
            for (ImageBorder border : borders) {
                assertThrows(RuntimeException.class, () -> hook.dense3x3(input, new GrayU8(20, 15), border));
                assertThrows(RuntimeException.class, () -> hook.dense5x5(input, new GrayS32(20, 15), border));
                Supplier<ImageBase> boofcv = () -> {
                    var output = new GrayS32(1, 1);
                    GCensusTransform.dense5x5(input, output, border);
                    return output;
                };
                assertIdentical(callBoofCV(null, boofcv), callBoofCV(hook, boofcv));
            }

            // More samples than bits in the census
            DogArray<Point2D_I32> samples = CensusTransform.createBlockSamples(4);
            assertThrows(RuntimeException.class, () -> hook.sample_S64(input, samples, new GrayS64(20, 15), null));
            Supplier<ImageBase> boofcv = () -> {
                var output = new GrayS64(1, 1);
                GCensusTransform.sample_S64(input, samples, output, null, null);
                return output;
            };
            assertIdentical(callBoofCV(null, boofcv), callBoofCV(hook, boofcv));
        }
    }

    /// Through BoofCV, sample_IU16 gives BoofCV's results whether or not the variant supports it
    @Test
    public void sampleIU16ThroughBoofCV() {
        for (Class type : imageTypes) {
            ImageGray input = randomImage(type, 20, 15);
            for (DogArray<Point2D_I32> samples : sampleSets()) {
                ImageBorder border = FactoryImageBorder.single(BorderType.REFLECT, type);
                Supplier<ImageBase> boofcv = () -> {
                    var output = new InterleavedU16(1, 1, 1);
                    GCensusTransform.sample_IU16(input, samples, output, border, null);
                    return output;
                };
                assertIdentical(callBoofCV(null, boofcv), callBoofCV(hook, boofcv));
            }
        }
    }

    interface Census {
        ImageBase apply( BOverrideCensusTransform.Census census, ImageGray input, ImageBorder border );
    }

    /// Compares the hook to BoofCV for every type, shape and border, called directly and through BoofCV's hook
    private void compare( Census op ) {
        BOverrideCensusTransform.Census boofcv = new JavaCensus();
        for (Class type : imageTypes) {
            for (int[] shape : SHAPES) {
                for (ImageBorder border : borders(type)) {
                    ImageGray input = randomImage(type, shape[0], shape[1]);
                    ImageBase expected = callBoofCV(null, () -> op.apply(boofcv, input, border));
                    assertIdentical(expected, op.apply(hook, input, border));
                    assertIdentical(expected, callBoofCV(hook, () -> op.apply(boofcv, input, border)));
                }
            }
        }
    }

    /// No border, extended, reflected, wrapped, zero and a constant value
    private static List<ImageBorder> borders( Class type ) {
        List<ImageBorder> list = new ArrayList<>();
        list.add(null);
        for (BorderType b : new BorderType[]{BorderType.EXTENDED, BorderType.REFLECT, BorderType.WRAP, BorderType.ZERO})
            list.add(FactoryImageBorder.single(b, type));
        list.add(FactoryImageBorder.singleValue(type == GrayF32.class ? 7.5 : 7, type));
        return list;
    }

    private static List<DogArray<Point2D_I32>> sampleSets() {
        var sparse = new DogArray<>(Point2D_I32::new);
        for (int[] p : new int[][]{{5, -1}, {0, 2}, {-3, 0}, {1, 1}, {-2, -6}})
            sparse.grow().setTo(p[0], p[1]);
        return Arrays.asList(CensusTransform.createBlockSamples(3), CensusTransform.createBlockSamples(4, 3),
                CensusTransform.createBlockSamples(6, 2), CensusTransform.createCircleSamples(), sparse);
    }

    /// Random sub-image with values in [0, 20], so neighbors are often equal. Floats have negative values and NaN,
    /// which compares false
    private ImageGray randomImage( Class type, int width, int height ) {
        ImageGray input = GeneralizedImageOps.createSingleBand(type, width, height);
        if (type == GrayF32.class) {
            GImageMiscOps.fillUniform(input, rand, -20, 20);
            ((GrayF32)input).data[rand.nextInt(width*height)] = Float.NaN;
        } else {
            GImageMiscOps.fillUniform(input, rand, 0, 20);
        }
        return BoofTesting.createSubImageOf(input);
    }

    /// Runs 'op' with 'census' registered with BoofCV. Null for BoofCV's Java code
    private static ImageBase callBoofCV( BOverrideCensusTransform.Census census, Supplier<ImageBase> op ) {
        BOverrideCensusTransform.Census registered = BOverrideCensusTransform.census;
        BOverrideCensusTransform.census = census;
        try {
            return op.get();
        } finally {
            BOverrideCensusTransform.census = registered;
        }
    }

    /// Census images are integers, and InterleavedU16 isn't an ImageGray, so BoofTesting compares them instead of
    /// assertMatches()
    private static void assertIdentical( ImageBase expected, ImageBase found ) {
        BoofTesting.assertEquals(expected, found, 0);
    }

    /// Calls BoofCV's census functions, which use the registered hook
    private static class JavaCensus implements BOverrideCensusTransform.Census {
        @Override public void dense3x3( ImageGray input, GrayU8 output, ImageBorder border ) {
            GCensusTransform.dense3x3(input, output, border);
        }

        @Override public void dense5x5( ImageGray input, GrayS32 output, ImageBorder border ) {
            GCensusTransform.dense5x5(input, output, border);
        }

        @Override public void sample_S64( ImageGray input, FastAccess sample, GrayS64 output,
                                          ImageBorder border ) {
            GCensusTransform.sample_S64(input, sample, output, border, null);
        }

        @Override public void sample_IU16( ImageGray input, FastAccess sample,
                                           InterleavedU16 output, ImageBorder border ) {
            GCensusTransform.sample_IU16(input, sample, output, border, null);
        }
    }
}
