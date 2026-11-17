package org.boofcpp.testing;

import boofcv.io.image.UtilImageIO;

import java.awt.image.BufferedImage;
import java.io.File;

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
