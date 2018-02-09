package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.testing.BoofTesting;
import org.boofcpp.BoofCPP;
import org.junit.Test;

import java.util.Random;

/**
 * -ea -Djava.library.path=/Users/pabeles/projects/BoofCPP/build/jni
 */
public class TestNativeLocalMean {
    Random rand = new Random(23423);

    Class types[] = new Class[]{GrayU8.class, GrayF32.class};

    static {
        BoofCPP.loadlib();
    }

    @Test
    public void compareToBoofCV() {
        for( Class type : types ) {
            // make the image larger to do a more careful test
            ImageGray input = GeneralizedImageOps.createSingleBand(type,400,300);
            GImageMiscOps.fillUniform(input,rand,0,255);

            GrayU8 found = new GrayU8(1,1);
            GrayU8 expected = new GrayU8( input.width, input.height);

            ConfigLength regionWidth = ConfigLength.fixed(25);
            double scale = 0.95;

            for( boolean down : new boolean[]{true,false}) {
                InputToBinary alg = new NativeLocalMean(regionWidth, scale, down, type);
                InputToBinary check = FactoryThresholdBinary.localMean(regionWidth, scale, down, type);
                
//                long time0 = System.currentTimeMillis();
                check.process(input, expected);
//                long time1 = System.currentTimeMillis();
                alg.process(input, found);
//                long time2 = System.currentTimeMillis();

//                System.out.println(type.getSimpleName()+" down="+down);
//                System.out.println("java "+(time1-time0)+" native "+(time2-time1));

                BoofTesting.assertEquals(expected, found, 0);
            }
        }

    }
}
