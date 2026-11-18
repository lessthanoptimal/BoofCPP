package org.boofcpp.accuracy;

import boofcv.alg.transform.census.BOverrideCensusTransform;
import boofcv.alg.transform.census.CensusTransform;
import boofcv.alg.transform.census.GCensusTransform;
import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS64;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import georegression.struct.point.Point2D_I32;
import org.boofcpp.testing.BenchmarkUtils;
import org.ddogleg.struct.DogArray;

/// Accuracy of a variant's census transform hook against BoofCV's Java code, on the test image with a reflected
/// border. Census images are integers, so every metric should be zero. sample_S64 samples BoofCV's 7x7 block and circle
@SuppressWarnings({"rawtypes", "unchecked"})
public class CensusAccuracy implements AccuracyCase {
    private final BOverrideCensusTransform.Census hook;
    private final Class[] imageTypes;

    public CensusAccuracy( BOverrideCensusTransform.Census hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    @Override public void run( AccuracyRecord record ) {
        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, gray.width, gray.height);
            GConvertImage.convert(gray, input);
            ImageBorder border = FactoryImageBorder.single(BorderType.REFLECT, type);
            String name = type.getSimpleName();

            var expected3 = new GrayU8(1, 1);
            var found3 = new GrayU8(input.width, input.height);
            withoutHook(() -> GCensusTransform.dense3x3(input, expected3, border));
            hook.dense3x3(input, found3, border);
            record.add("census_3x3", name, "test_image", ImageDifference.measure(expected3, found3));

            var expected5 = new GrayS32(1, 1);
            var found5 = new GrayS32(input.width, input.height);
            withoutHook(() -> GCensusTransform.dense5x5(input, expected5, border));
            hook.dense5x5(input, found5, border);
            record.add("census_5x5", name, "test_image", ImageDifference.measure(expected5, found5));

            for (String region : new String[]{"block_7x7", "circle_9"}) {
                DogArray<Point2D_I32> samples = region.equals("block_7x7") ?
                        CensusTransform.createBlockSamples(3) : CensusTransform.createCircleSamples();
                var expected = new GrayS64(1, 1);
                var found = new GrayS64(input.width, input.height);
                withoutHook(() -> GCensusTransform.sample_S64(input, samples, expected, border, null));
                hook.sample_S64(input, samples, found, border);
                record.add("census_sample", name, "test_image " + region, ImageDifference.measure(expected, found));
            }
        }
    }

    /// Runs BoofCV's Java code, not a registered hook
    private static void withoutHook( Runnable boofcv ) {
        BOverrideCensusTransform.Census registered = BOverrideCensusTransform.census;
        BOverrideCensusTransform.census = null;
        try {
            boofcv.run();
        } finally {
            BOverrideCensusTransform.census = registered;
        }
    }
}
