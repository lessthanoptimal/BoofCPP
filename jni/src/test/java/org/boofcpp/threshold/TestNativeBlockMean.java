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
import org.junit.Test;

import java.util.Random;

/**
 * -ea -Djava.library.path=/Users/pabeles/projects/BoofCPP/build/jni
 */
public class TestNativeBlockMean {
    Random rand = new Random(23423);

    Class types[] = new Class[]{GrayU8.class, GrayF32.class};

    static {
        System.loadLibrary("JNIBoofCPP");
    }

    @Test
    public void compareToBoofCV() {
        for( Class type : types ) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type,300,200);
            GImageMiscOps.fillUniform(input,rand,0,255);

            GrayU8 found = new GrayU8(1,1);
            GrayU8 expected = new GrayU8( input.width, input.height);

            ConfigLength regionWidth = ConfigLength.fixed(25);
            double scale = 0.95;

            for( boolean down : new boolean[]{true,false}) {
                for( boolean useLocal : new boolean[]{true,false}) {
                    InputToBinary alg = new NativeBlockMean(regionWidth, scale, down, useLocal, type);
                    InputToBinary check = FactoryThresholdBinary.blockMean(regionWidth, scale, down, useLocal, type);

                    check.process(input, expected);
                    alg.process(input, found);

//                System.out.println(type.getSimpleName()+" "+down);
//                expected.print();
//                System.out.println();
//                found.print();

                    BoofTesting.assertEquals(expected, found, 0);
                }
            }
        }

    }
}
