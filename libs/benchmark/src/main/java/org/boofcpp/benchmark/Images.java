package org.boofcpp.benchmark;

import boofcv.core.image.GConvertImage;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageDataType;
import boofcv.struct.image.ImageGray;
import org.boofcpp.testing.BenchmarkUtils;

/// Input images of the benchmarks
public class Images {
    /// The test image, BenchmarkUtils.TEST_IMAGE, as an image of 'type', e.g. "U8"
    public static ImageGray testImage( String type ) {
        GrayU8 gray = ConvertBufferedImage.convertFrom(BenchmarkUtils.loadImage(BenchmarkUtils.TEST_IMAGE),
                (GrayU8)null);
        ImageGray image = GeneralizedImageOps.createSingleBand(ImageDataType.valueOf(type), gray.width, gray.height);
        GConvertImage.convert(gray, image);
        return image;
    }
}
