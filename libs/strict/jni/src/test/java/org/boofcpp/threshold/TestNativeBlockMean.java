package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.BoofTesting;
import org.boofcpp.BoofCPP;
import org.junit.jupiter.api.Test;

import java.util.Random;

/**
 * -ea -Djava.library.path=/Users/pabeles/projects/BoofCPP/build/jni
 */
public class TestNativeBlockMean {
    Random rand = new Random(23423);

    Class types[] = new Class[]{GrayU8.class, GrayF32.class};

    static {
        BoofCPP.loadlib();
    }

    @Test
    public void compareToBoofCV() {
        // Even and odd shapes (partial blocks), whole image and sub-image (offset/stride)
        for (int[] shape : new int[][]{{400,300},{401,297}})
        for (boolean sub : new boolean[]{false,true})
        for( Class type : types ) {
            // make the image larger to do a more careful test
            ImageGray input = GeneralizedImageOps.createSingleBand(type,shape[0],shape[1]);
            GImageMiscOps.fillUniform(input,rand,0,255);
            if (sub) input = BoofTesting.createSubImageOf(input);

            GrayU8 found = new GrayU8(1,1);
            GrayU8 expected = new GrayU8( input.width, input.height);

            double scale = 0.95;

            for (ConfigLength regionWidth : new ConfigLength[]{ConfigLength.fixed(25), ConfigLength.fixed(7), ConfigLength.relative(0.05,5)})
            for( boolean down : new boolean[]{true,false}) {
                for( boolean useLocal : new boolean[]{true,false}) {
                    InputToBinary alg = new NativeBlockMean(regionWidth, scale, down, useLocal, type);
                    InputToBinary check = FactoryThresholdBinary.blockMean(regionWidth, scale, down, useLocal, type);

//                    long time0 = System.currentTimeMillis();
                    check.process(input, expected);
//                    long time1 = System.currentTimeMillis();
                    alg.process(input, found);
//                    long time2 = System.currentTimeMillis();

//                    System.out.println(type.getSimpleName()+" down="+down+" local="+useLocal);
//                    System.out.println("java "+(time1-time0)+" native "+(time2-time1));
//                    expected.print();
//                    System.out.println();
//                    found.print();

                    BoofTesting.assertEquals(expected, found, 0);
                }
            }
        }

    }
}
