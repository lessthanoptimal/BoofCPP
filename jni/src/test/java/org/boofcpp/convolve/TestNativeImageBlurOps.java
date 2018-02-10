package org.boofcpp.convolve;

import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.testing.BoofTesting;
import georegression.misc.GrlConstants;
import org.boofcpp.BoofCPP;
import org.junit.Test;

import java.util.Random;

public class TestNativeImageBlurOps {
    Random rand = new Random(23423);

    Class types[] = new Class[]{GrayU8.class, GrayF32.class};

    int width=400,height=300;

    static {
        BoofCPP.loadlib();
    }

    @Test
    public void compareMeanToBoof() {
        NativeImageBlurOps nativeBlur = new NativeImageBlurOps();

        for( Class type : types ) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray found = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray expected = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray storage = GeneralizedImageOps.createSingleBand(type,width,height);

            GImageMiscOps.fillUniform(input,rand,0,255);

            for( int radius : new int[]{1,2,5,24} ) {
//                for (int i = 0; i < 5; i++) {
//                long time0 = System.currentTimeMillis();
                nativeBlur.processMean(input,found,radius,storage);
//                long time1 = System.currentTimeMillis();
                GBlurImageOps.mean(input,expected,radius,storage);
//                long time2 = System.currentTimeMillis();

//                System.out.println(input.getDataType() + " radius " + radius + " native " + (time1 - time0) + "  java " + (time2 - time1));

                BoofTesting.assertEquals(expected, found, GrlConstants.TEST_F32);
//                }
            }
        }
    }
}
