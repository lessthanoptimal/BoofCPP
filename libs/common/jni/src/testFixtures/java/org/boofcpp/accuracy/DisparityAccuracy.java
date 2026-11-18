package org.boofcpp.accuracy;

import boofcv.abst.disparity.StereoDisparity;
import boofcv.core.image.ConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.disparity.BOverrideStereoDisparity;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.disparity.FactoryStereoDisparity;
import boofcv.factory.transform.census.CensusVariants;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.BenchmarkUtils;

/// Accuracy of a variant's stereo block matching against BoofCV's Java code, on the benchmarks' stereo pair with
/// BoofCV's default configuration and each error type. A near tie can select another disparity, so two errors are
/// recorded instead of the disparity's: op "disparity_score" is the score of the selected disparity where both are
/// valid, and op "disparity_valid" compares which pixels are valid, as images of 0 and 1, so its differing is the
/// fraction of pixels which don't agree
@SuppressWarnings({"rawtypes", "unchecked"})
public class DisparityAccuracy implements AccuracyCase {
    private final BOverrideStereoDisparity.CreateBlockMatch hook;
    private final Class[] imageTypes;

    public DisparityAccuracy( BOverrideStereoDisparity.CreateBlockMatch hook, Class... imageTypes ) {
        this.hook = hook;
        this.imageTypes = imageTypes;
    }

    @Override public void run( AccuracyRecord record ) {
        GrayU8[] pair = BenchmarkUtils.stereoPair();
        for (Class type : imageTypes) {
            ImageGray left = pair[0], right = pair[1];
            if (type == GrayF32.class) {
                left = ConvertImage.convert(pair[0], (GrayF32)null);
                right = ConvertImage.convert(pair[1], (GrayF32)null);
            }
            for (String error : new String[]{"SAD", "SSD", "ZSSD", "NCC", "CENSUS_5_5", "CENSUS_7_7"}) {
                var config = new ConfigDisparityBM();
                if (error.startsWith("CENSUS")) {
                    config.errorType = DisparityError.CENSUS;
                    config.configCensus.variant = CensusVariants.valueOf("BLOCK" + error.substring(6));
                } else {
                    config.errorType = DisparityError.valueOf(error);
                }
                StereoDisparity expected = withoutHook(config, type);
                StereoDisparity found = hook.create(config, type, GrayF32.class);
                expected.process(left, right);
                found.process(left, right);

                GrayU8 validExpected = valid(expected), validFound = valid(found);
                var both = new GrayU8(left.width, left.height);
                for (int i = 0; i < both.data.length; i++)
                    both.data[i] = (byte)(validExpected.data[i] & validFound.data[i]);
                String name = type.getSimpleName(), input = "stereo_pair " + error.toLowerCase();
                record.add("disparity_score", name, input, ImageDifference.measureWhere(
                        expected.getDisparityScore(), found.getDisparityScore(), both));
                record.add("disparity_valid", name, input, ImageDifference.measure(validExpected, validFound));
            }
        }
    }

    /// 1 where the disparity is valid, otherwise 0
    private static GrayU8 valid( StereoDisparity alg ) {
        ImageGray disparity = alg.getDisparity();
        var valid = new GrayU8(disparity.width, disparity.height);
        for (int y = 0; y < disparity.height; y++) {
            for (int x = 0; x < disparity.width; x++) {
                valid.set(x, y, GeneralizedImageOps.get(disparity, x, y) < alg.getInvalidValue() ? 1 : 0);
            }
        }
        return valid;
    }

    /// BoofCV's Java implementation, not a registered hook
    private static StereoDisparity withoutHook( ConfigDisparityBM config, Class type ) {
        BOverrideStereoDisparity.CreateBlockMatch registered = BOverrideStereoDisparity.opCreateBlockMatch;
        BOverrideStereoDisparity.opCreateBlockMatch = null;
        try {
            return FactoryStereoDisparity.blockMatch(config, type, GrayF32.class);
        } finally {
            BOverrideStereoDisparity.opCreateBlockMatch = registered;
        }
    }
}
