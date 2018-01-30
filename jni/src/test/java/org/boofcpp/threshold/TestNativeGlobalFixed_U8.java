package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.struct.image.GrayU8;
import boofcv.testing.BoofTesting;
import org.junit.Test;

import java.util.Random;

/**
 *
 */
public class TestNativeGlobalFixed_U8 {
    Random rand = new Random(23423);

    static {
        System.loadLibrary("JNIBoofCPP");
    }

    @Test
    public void compareToBoofCV() {
        GrayU8 input = new GrayU8(5,6);
        GImageMiscOps.fillUniform(input,rand,0,255);

        GrayU8 found = new GrayU8(1,1);
        GrayU8 expected = input.createSameShape();

        double threshold = 125;

        for( boolean down : new boolean[]{true,false}) {
            InputToBinary<GrayU8> alg = new NativeGlobalFixed_U8(threshold, down);
            InputToBinary<GrayU8> check = FactoryThresholdBinary.globalFixed(threshold, down, GrayU8.class);

            check.process(input, expected);
            alg.process(input, found);

//            expected.print();
//            System.out.println();
//            found.print();

            BoofTesting.assertEquals(expected, found, 0);
        }
    }
}
