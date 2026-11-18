package org.boofcpp.testing;

import boofcv.alg.filter.misc.AverageDownSampleOps;
import boofcv.io.image.ConvertBufferedImage;
import boofcv.io.image.UtilImageIO;
import boofcv.struct.image.GrayU8;

import java.awt.image.BufferedImage;
import java.io.File;
import java.util.Random;

/// Loads the images the benchmarks and the accuracy tool use
public class BenchmarkUtils {
    /// Path of the test image, relative to the project root
    public static final String TEST_IMAGE = "datasets/test_image.jpg";

    public static BufferedImage loadImage( String path )
    {
        File f = new File(findProjectRoot(),path);
        if( !f.exists() ) {
            throw new IllegalArgumentException("Does not exist: "+f.getPath());
        }
        BufferedImage buffered = UtilImageIO.loadImage(f.getPath());
        if( buffered == null )
            throw new RuntimeException("Couldn't load the image at "+f.getPath());
        return buffered;
    }

    /// Synthetic stereo pair for the block matching benchmarks: the test image downsampled by 6 to 672x504, and the
    /// left image shifted by [#STEREO_SHIFT] pixels plus uniform noise in `[-3,3]` as the right image
    public static GrayU8[] stereoPair() {
        GrayU8 full = ConvertBufferedImage.convertFrom(loadImage(TEST_IMAGE), (GrayU8)null);
        GrayU8 left = new GrayU8(full.width/6, full.height/6);
        AverageDownSampleOps.down(full, 6, left);
        var rand = new Random(0xBEEF);
        GrayU8 right = left.createSameShape();
        for (int y = 0; y < left.height; y++) {
            for (int x = 0; x < left.width; x++) {
                int v = left.get(Math.min(left.width - 1, x + STEREO_SHIFT), y) + rand.nextInt(7) - 3;
                right.set(x, y, Math.max(0, Math.min(255, v)));
            }
        }
        return new GrayU8[]{left, right};
    }

    /// Disparity of the stereo pair from [#stereoPair()]
    public static final int STEREO_SHIFT = 16;

    public static File findProjectRoot() {
        File dir = new File(".").getAbsoluteFile();

        while( dir != null ) {
            File files[] = dir.listFiles();
            if( files != null ) {
                boolean foundLibs = false;
                boolean foundChange = false;
                boolean foundReadMe = false;

                for (File f : files) {
                    switch (f.getName()) {
                        case "libs":
                            foundLibs = true;
                            break;

                        case "change.md":
                            foundChange = true;
                            break;

                        case "readme.md":
                            foundReadMe = true;
                            break;
                    }
                }

                if (foundLibs && foundChange && foundReadMe)
                    return dir;
            }
            dir = dir.getParentFile();
        }
        throw new RuntimeException("Can't find project root. pwd="+new File(".").getAbsolutePath());
    }
}
