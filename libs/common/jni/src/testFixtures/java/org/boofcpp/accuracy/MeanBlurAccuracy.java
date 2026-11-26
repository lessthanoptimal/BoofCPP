package org.boofcpp.accuracy;

import boofcv.alg.filter.blur.BOverrideBlurImageOps;
import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.BenchmarkUtils;

import java.util.Random;

/// Accuracy of a variant's mean blur hook, without a border (op "mean") and with one (op "mean_border"), against
/// BoofCV's Java code. Inputs are the test image and, for floats, ones which are hard for floating point: long running
/// sums and large values. The border is reflected, and the test image is also run with every other border
@SuppressWarnings({"rawtypes", "unchecked"})
public class MeanBlurAccuracy implements AccuracyCase {
    private final BOverrideBlurImageOps.Mean hook;
    private final Class[] imageTypes;

    public MeanBlurAccuracy( BOverrideBlurImageOps.Mean hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    @Override public void run( AccuracyRecord record ) {
        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        for (Class type : imageTypes) {
            ImageGray image = GeneralizedImageOps.createSingleBand(type, gray.width, gray.height);
            GConvertImage.convert(gray, image);
            for (int radius : new int[]{1, 5, 30}) {
                compare(record, image, radius, radius, "test_image r=" + radius);
            }
            for (BorderType border : new BorderType[]{BorderType.EXTENDED, BorderType.WRAP, BorderType.ZERO}) {
                compareBorder(record, image, 5, 5, border, "test_image r=5 " + border);
            }

            if (type != GrayF32.class && type != GrayF64.class)
                continue;
            Random rand = new Random(234);
            compare(record, random(type, 2001, 7, 255, rand), 30, 2, "long_rows");
            compare(record, random(type, 7, 2001, 255, rand), 2, 30, "long_columns");
            compare(record, random(type, 203, 101, 1e5, rand), 3, 4, "large_values");
        }
    }

    /// Without a border, then with a reflected border
    private void compare( AccuracyRecord record, ImageGray input, int radiusX, int radiusY, String name ) {
        ImageGray expected = (ImageGray)input.createSameShape();
        ImageGray found = (ImageGray)input.createSameShape();
        withoutHook(() -> GBlurImageOps.mean(input, expected, radiusX, radiusY, null));
        hook.processMean(input, found, radiusX, radiusY, BorderType.NORMALIZED, null);
        record.add("mean", input.getClass().getSimpleName(), name, ImageDifference.measure(expected, found));
        compareBorder(record, input, radiusX, radiusY, BorderType.REFLECT, name);
    }

    private void compareBorder( AccuracyRecord record, ImageGray input, int radiusX, int radiusY, BorderType border,
                                String name ) {
        Class type = input.getClass();
        ImageGray expected = (ImageGray)input.createSameShape();
        ImageGray found = (ImageGray)input.createSameShape();
        ImageBorder javaBorder = FactoryImageBorder.single(border, type);
        withoutHook(() -> GBlurImageOps.meanB(input, expected, radiusX, radiusY, javaBorder, null));
        hook.processMean(input, found, radiusX, radiusY, border, null);
        record.add("mean_border", type.getSimpleName(), name, ImageDifference.measure(expected, found));
    }

    /// Runs BoofCV's Java code, not a registered hook
    private static void withoutHook( Runnable boofcv ) {
        BOverrideBlurImageOps.Mean registered = BOverrideBlurImageOps.mean;
        BOverrideBlurImageOps.mean = null;
        try {
            boofcv.run();
        } finally {
            BOverrideBlurImageOps.mean = registered;
        }
    }

    private static ImageGray random( Class type, int width, int height, double max, Random rand ) {
        ImageGray image = GeneralizedImageOps.createSingleBand(type, width, height);
        GImageMiscOps.fillUniform(image, rand, 0, max);
        return image;
    }
}
