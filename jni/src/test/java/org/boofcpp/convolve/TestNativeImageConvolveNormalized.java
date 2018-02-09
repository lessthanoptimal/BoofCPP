package org.boofcpp.convolve;

import boofcv.alg.filter.blur.GBlurImageOps;
import boofcv.alg.filter.convolve.ConvolveNormalized;
import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.testing.BoofTesting;
import georegression.misc.GrlConstants;
import org.boofcpp.BoofCPP;
import org.junit.Test;

import java.util.Random;

public class TestNativeImageConvolveNormalized {
    Random rand = new Random(23423);

    Class types[] = new Class[]{GrayU8.class, GrayF32.class};

    int width=400,height=300;

    static {
        BoofCPP.loadlib();
    }

    @Test
    public void compareHorizontalToBoof() {
        NativeImageConvolveNormalized nativeConv = new NativeImageConvolveNormalized();

        for( Class type : types ) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray found = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray expected = GeneralizedImageOps.createSingleBand(type,width,height);

            GImageMiscOps.fillUniform(input,rand,0,255);

            for( int radius : new int[]{1,2,5} ) {
                Kernel1D kernel = FactoryKernelGaussian.gaussian1D(type,-1,radius);

                nativeConv.horizontal(kernel,input,found);
                GConvolveImageOps.horizontalNormalized(kernel,input,expected);

                BoofTesting.assertEquals(expected, found, GrlConstants.TEST_F32);
            }

        }
    }

    @Test
    public void compareVerticalToBoof() {
        NativeImageConvolveNormalized nativeConv = new NativeImageConvolveNormalized();

        for( Class type : types ) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray found = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray expected = GeneralizedImageOps.createSingleBand(type,width,height);

            GImageMiscOps.fillUniform(input,rand,0,255);

            for( int radius : new int[]{1,2,5} ) {
                Kernel1D kernel = FactoryKernelGaussian.gaussian1D(type,-1,radius);

                nativeConv.vertical(kernel,input,found);
                GConvolveImageOps.verticalNormalized(kernel,input,expected);

                BoofTesting.assertEquals(expected, found, GrlConstants.TEST_F32);
            }

        }
    }
}
