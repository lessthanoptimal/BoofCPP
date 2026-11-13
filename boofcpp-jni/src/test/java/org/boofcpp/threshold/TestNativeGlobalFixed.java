package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
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
public class TestNativeGlobalFixed {
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
        for( Class imageType : types ) {
            ImageGray input = GeneralizedImageOps.createSingleBand(imageType,shape[0],shape[1]);
            GImageMiscOps.fillUniform(input,rand,0,255);
            if (sub) input = BoofTesting.createSubImageOf(input);

            GrayU8 found = new GrayU8(1,1);
            GrayU8 expected = new GrayU8( input.width, input.height);

            // includes fractional and outside of pixel range thresholds
            for (double threshold : new double[]{125, 125.5, -1, 0, 255, 300})
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
