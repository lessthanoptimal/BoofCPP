package org.boofcpp.accuracy;

import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.AverageDownSampleHooks;
import org.boofcpp.testing.BenchmarkUtils;

import java.util.Arrays;
import java.util.Random;

/// Accuracy of a variant's average down sampling hooks against BoofCV's Java code: square regions (op "down") and
/// regions with the output's size (op "down_general"). Inputs are the test image and, for floats, large values with
/// both signs
@SuppressWarnings({"rawtypes", "unchecked"})
public class AverageDownSampleAccuracy implements AccuracyCase {
    private final BOverrideAverageDownSampleOps.Square square;
    private final BOverrideAverageDownSampleOps.General general;
    private final Class[] imageTypes;

    public <H extends BOverrideAverageDownSampleOps.Square & BOverrideAverageDownSampleOps.General>
    AverageDownSampleAccuracy( H hook, Class... imageTypes ) {
        this.square = hook;
        this.general = hook;
        this.imageTypes = imageTypes;
    }

    @Override public void run( AccuracyRecord record ) {
        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        for (Class type : imageTypes) {
            ImageGray image = GeneralizedImageOps.createSingleBand(type, gray.width, gray.height);
            GConvertImage.convert(gray, image);
            compare(record, image, "test_image");

            if (type != GrayF32.class && type != GrayF64.class)
                continue;
            ImageGray large = GeneralizedImageOps.createSingleBand(type, 1001, 999);
            GImageMiscOps.fillUniform(large, new Random(234), -1e6, 1e6);
            compare(record, large, "large_values");
        }
    }

    private void compare( AccuracyRecord record, ImageGray input, String name ) {
        String type = input.getClass().getSimpleName();
        for (int width : new int[]{2, 3, 5, 8}) {
            ImageGray expected = GeneralizedImageOps.createSingleBand(input.getClass(),
                    AverageDownSampleOps.downSampleSize(input.width, width),
                    AverageDownSampleOps.downSampleSize(input.height, width));
            ImageGray found = (ImageGray)expected.createSameShape();
            AverageDownSampleHooks.withoutHooks(() -> AverageDownSampleOps.down(input, width, expected));
            square.processDown(input, width, found);
            record.add("down", type, name + " width=" + width, ImageDifference.measure(expected, found));
        }

        if (!Arrays.asList(GrayU8.class, GrayU16.class, GrayF32.class, GrayF64.class).contains(input.getClass()))
            return;
        for (int divisor : new int[]{2, 3}) {
            for (boolean centered : new boolean[]{false, true}) {
                ImageGray expected = GeneralizedImageOps.createSingleBand(input.getClass(), input.width/divisor,
                        input.height/divisor);
                ImageGray found = (ImageGray)expected.createSameShape();
                AverageDownSampleHooks.withoutHooks(() -> AverageDownSampleOps.down(input, centered, expected, null));
                general.processDown(input, centered, found, null);
                record.add("down_general", type, name + " size=1/" + divisor + " centered=" + centered,
                        ImageDifference.measure(expected, found));
            }
        }
    }
}
