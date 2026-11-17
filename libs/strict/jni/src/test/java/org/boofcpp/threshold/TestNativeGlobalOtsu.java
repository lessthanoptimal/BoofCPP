package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.binary.FactoryThresholdBinary;
import boofcv.abst.filter.binary.GlobalBinaryFilter;
import boofcv.factory.filter.binary.BOverrideFactoryThresholdBinary;
import boofcv.struct.image.ImageType;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.BoofTesting;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.Test;

/**
 * -ea -Djava.library.path=/Users/pabeles/projects/BoofCPP/build/jni
 */
public class TestNativeGlobalOtsu extends BoofCPPStandardJUnit {
    Class types[] = new Class[]{GrayU8.class, GrayF32.class};

    static {
        BoofCPP.loadlib();
    }

    @Test
    public void compareToBoofCV() {
        for( Class type : types ) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type,100,90);
            GrayU8 found = new GrayU8(1,1);
            GrayU8 expected = new GrayU8( input.width, input.height);

            // pixel value range. min != 0 shifts the histogram
            for (double[] range : new double[][]{{0,255},{20,200}}) {
            double minValue = range[0];
            double maxValue = range[1];
            GImageMiscOps.fillUniform(input,rand,minValue,maxValue);

            // scale values chosen so that the threshold can go below 0 and above 255
            for( double scale : new double[]{1.0, 0.0, 0.5, 0.95, 1.15, 3.0} )
            for( boolean down : new boolean[]{true,false}) {
                InputToBinary alg = new NativeGlobalOtsu(minValue,maxValue, scale, down, type);
                InputToBinary check = FactoryThresholdBinary.globalOtsu(minValue, maxValue, scale, down, type);

                check.process(input, expected);
                alg.process(input, found);

                BoofTesting.assertEquals(expected, found, 0);
            }
            }
        }
    }

    /**
     * Checks that the factory passes all arguments, including scale, through the override hook
     */
    @Test
    public void factoryOverrideHook() {
        for( Class type : types ) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type,100,90);
            GImageMiscOps.fillUniform(input,rand,0,255);

            GrayU8 found = new GrayU8(1,1);
            GrayU8 expected = new GrayU8( input.width, input.height);

            for( double scale : new double[]{1.0, 0.7, 1.3} ) {
                InputToBinary check = new GlobalBinaryFilter.Otsu(0, 255, scale, true, ImageType.single(type));
                InputToBinary alg;
                try {
                    BOverrideFactoryThresholdBinary.globalOtsu = NativeGlobalOtsu::new;
                    alg = FactoryThresholdBinary.globalOtsu(0, 255, scale, true, type);
                } finally {
                    BOverrideFactoryThresholdBinary.globalOtsu = null;
                }
                assertTrue(alg instanceof NativeGlobalOtsu);

                check.process(input, expected);
                alg.process(input, found);
                BoofTesting.assertEquals(expected, found, 0);
            }
        }
    }

    /// Pixels outside [minValue, maxValue] are handled as in BoofCV. F32 values less than 1 outside and NaN are
    /// counted, others throw
    @Test
    public void pixelsOutsideRange() {
        GrayF32 inputF = new GrayF32(30, 20);
        GImageMiscOps.fillUniform(inputF, rand, 20, 200);
        inputF.set(0, 0, 19.5f);
        inputF.set(1, 0, 200.9f);
        inputF.set(2, 0, Float.NaN);
        GrayU8 expected = new GrayU8(30, 20);
        GrayU8 found = new GrayU8(30, 20);
        new GlobalBinaryFilter.Otsu<>(20, 200, 1.0, true, ImageType.single(GrayF32.class)).process(inputF, expected);
        new NativeGlobalOtsu<>(20, 200, 1.0, true, GrayF32.class).process(inputF, found);
        BoofTesting.assertEquals(expected, found, 0);

        for (float value : new float[]{19f, 201f, Float.POSITIVE_INFINITY}) {
            inputF.set(2, 0, value);
            checkBothThrow(inputF, GrayF32.class);
        }

        GrayU8 inputU = new GrayU8(30, 20);
        GImageMiscOps.fillUniform(inputU, rand, 20, 200);
        for (int value : new int[]{19, 201, 0, 255}) {
            inputU.set(2, 0, value);
            checkBothThrow(inputU, GrayU8.class);
        }
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    private void checkBothThrow( ImageGray input, Class type ) {
        GrayU8 output = new GrayU8(input.width, input.height);
        assertThrows(RuntimeException.class,
                () -> new GlobalBinaryFilter.Otsu(20, 200, 1.0, true, ImageType.single(type)).process(input, output));
        assertThrows(RuntimeException.class, () -> new NativeGlobalOtsu(20, 200, 1.0, true, type).process(input, output));
    }
}
