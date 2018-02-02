package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
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
public class TestNativeGlobalFixed {
    Random rand = new Random(23423);
    Class types[] = new Class[]{GrayU8.class, GrayF32.class};

    static {
        BoofCPP.loadlib();
    }

    @Test
    public void compareToBoofCV() {
        double threshold = 125;

        for( Class imageType : types ) {
            ImageGray input = GeneralizedImageOps.createSingleBand(imageType,400,300);
            GImageMiscOps.fillUniform(input,rand,0,255);

            GrayU8 found = new GrayU8(1,1);
            GrayU8 expected = new GrayU8( input.width, input.height);

            for (boolean down : new boolean[]{true, false}) {
                InputToBinary alg = new NativeGlobalFixed(threshold, down, imageType);
                InputToBinary check = FactoryThresholdBinary.globalFixed(threshold, down, imageType);

                check.process(input, expected);
                alg.process(input, found);

//            expected.print();
//            System.out.println();
//            found.print();

                BoofTesting.assertEquals(expected, found, 0);
            }
        }
    }
}
