package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel1D_F32;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.BoofTesting;
import org.boofcpp.BoofCPP;
import org.junit.jupiter.api.Test;

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

            input = BoofTesting.createSubImageOf(input); // non-zero offset/stride


            for( int radius : new int[]{1,2,3,4,5,24} )
            for( int kscale : new int[]{1,2} ) {
                Kernel1D kernel = scale(FactoryKernelGaussian.gaussian1D(type, -1, radius),kscale);

//                long time0 = System.currentTimeMillis();
                nativeConv.horizontal(kernel, input, found);
//                long time1 = System.currentTimeMillis();
                GConvolveImageOps.horizontalNormalized(kernel, input, expected);
//                long time2 = System.currentTimeMillis();

//                System.out.println(input.getDataType() + " radius " + radius + " native " + (time1 - time0) + "  java " + (time2 - time1));

                BoofTesting.assertEquals(expected, found, 0); // bit identical to BoofCV
//                }
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

            input = BoofTesting.createSubImageOf(input); // non-zero offset/stride

            for( int radius : new int[]{1,2,3,4,5,24} )
            for( int kscale : new int[]{1,2} ) {
                Kernel1D kernel = scale(FactoryKernelGaussian.gaussian1D(type,-1,radius),kscale);

                nativeConv.vertical(kernel,input,found);
                GConvolveImageOps.verticalNormalized(kernel,input,expected);

                BoofTesting.assertEquals(expected, found, 0); // bit identical to BoofCV
            }

        }
    }

    /**
     * Scales the kernel so that its sum is no longer one. BoofCV then normalizes the kernel before convolving.
     */
    static Kernel1D scale( Kernel1D kernel, int scale ) {
        if( kernel instanceof Kernel1D_F32 ) {
            float[] d = ((Kernel1D_F32)kernel).data;
            for (int i = 0; i < d.length; i++) d[i] *= scale;
        } else {
            int[] d = ((Kernel1D_S32)kernel).data;
            for (int i = 0; i < d.length; i++) d[i] *= scale;
        }
        return kernel;
    }
}
