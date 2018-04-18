package org.boofcpp;

import boofcv.io.image.UtilImageIO;

import java.awt.image.BufferedImage;
import java.io.File;

public class BenchmarkUtils {
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
                boolean foundSrcJni = false;
                boolean foundChange = false;
                boolean foundReadMe = false;

                for (File f : files) {
                    switch (f.getName()) {
                        case "boofcpp-jni":
                            foundSrcJni = true;
                            break;

                        case "change.md":
                            foundChange = true;
                            break;

                        case "readme.md":
                            foundReadMe = true;
                            break;
                    }
                }

                if (foundSrcJni && foundChange && foundReadMe)
                    return dir;
            }
            dir = dir.getParentFile();
        }
        throw new RuntimeException("Can't find project root. pwd="+new File(".").getAbsolutePath());
    }
}
