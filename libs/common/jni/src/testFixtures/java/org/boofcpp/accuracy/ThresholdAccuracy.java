package org.boofcpp.accuracy;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.BenchmarkUtils;

import java.util.function.Supplier;

/// Threshold filters on the test image, compared with BoofCV's Java filters with the same settings as the JMH
/// benchmark. The mean difference of the binary images is the fraction of pixels which differ. Filters whose hook is
/// null aren't measured
@SuppressWarnings({"rawtypes", "unchecked"})
public class ThresholdAccuracy implements AccuracyCase {
    private final BOverrideFactoryThresholdBinary.GlobalOtsu globalOtsu;
    private final BOverrideFactoryThresholdBinary.LocalMean localMean;
    private final BOverrideFactoryThresholdBinary.LocalBlockMean blockMean;
    private final BOverrideFactoryThresholdBinary.LocalBlockOtsu blockOtsu;
    private final BOverrideFactoryThresholdBinary.LocalBlockMinMax blockMinMax;
    private final Class[] imageTypes;

    public ThresholdAccuracy( BOverrideFactoryThresholdBinary.GlobalOtsu globalOtsu,
                              BOverrideFactoryThresholdBinary.LocalMean localMean,
                              BOverrideFactoryThresholdBinary.LocalBlockMean blockMean,
                              BOverrideFactoryThresholdBinary.LocalBlockOtsu blockOtsu,
                              BOverrideFactoryThresholdBinary.LocalBlockMinMax blockMinMax,
                              Class... imageTypes ) {
        this.globalOtsu = globalOtsu;
        this.localMean = localMean;
        this.blockMean = blockMean;
        this.blockOtsu = blockOtsu;
        this.blockMinMax = blockMinMax;
        this.imageTypes = imageTypes;
    }

    @Override public void run( AccuracyRecord record ) {
        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        ConfigLength width = ConfigLength.fixed(40);
        double scale = 0.95;
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, gray.width, gray.height);
            GConvertImage.convert(gray, input);
            String name = type.getSimpleName();

            if (globalOtsu != null)
                measure(record, "global_otsu", name, "test_image", input,
                        () -> globalOtsu.handle(0, 255, 1.0, true, type),
                        () -> FactoryThresholdBinary.globalOtsu(0, 255, 1.0, true, type));
            if (localMean != null)
                measure(record, "local_mean", name, "test_image", input,
                        () -> localMean.handle(width, scale, true, type),
                        () -> FactoryThresholdBinary.localMean(width, scale, true, type));
            for (boolean local : new boolean[]{false, true}) {
                String image = "test_image local_blocks=" + local;
                if (blockMean != null)
                    measure(record, "block_mean", name, image, input,
                            () -> blockMean.handle(width, scale, true, local, type),
                            () -> FactoryThresholdBinary.blockMean(width, scale, true, local, type));
                if (blockOtsu != null)
                    measure(record, "block_otsu", name, image, input,
                            () -> blockOtsu.handle(true, width, 0, scale, true, local, type),
                            () -> FactoryThresholdBinary.blockOtsu(width, scale, true, local, true, 0, type));
                if (blockMinMax != null)
                    measure(record, "block_minmax", name, image, input,
                            () -> blockMinMax.handle(width, scale, true, 1, local, type),
                            () -> FactoryThresholdBinary.blockMinMax(width, scale, true, local, 1, type));
            }
        }
    }

    private static void measure( AccuracyRecord record, String op, String type, String image, ImageGray input,
                                 Supplier<InputToBinary> variant, Supplier<InputToBinary> boofcv ) {
        var expected = new GrayU8(input.width, input.height);
        var found = new GrayU8(input.width, input.height);
        boofcv.get().process(input, expected);
        variant.get().process(input, found);
        record.add(op, type, image, ImageDifference.measure(expected, found));
    }
}
