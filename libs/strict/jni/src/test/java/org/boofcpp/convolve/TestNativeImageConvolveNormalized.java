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
import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.assertThrows;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayS8;

public class TestNativeImageConvolveNormalized extends BoofCPPStandardJUnit {
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

    /// Types native doesn't support throw a RuntimeException, so BoofCV uses its Java code
    @Test
    public void unsupportedTypesThrow() {
        NativeImageConvolveNormalized alg = new NativeImageConvolveNormalized();
        Kernel1D_S32 kernel = new Kernel1D_S32(3, new int[]{1, 2, 1});
        assertThrows(RuntimeException.class, () -> alg.horizontal(kernel, new GrayS8(10, 8), new GrayS8(10, 8)));
        assertThrows(RuntimeException.class, () -> alg.vertical(kernel, new GrayU16(10, 8), new GrayU16(10, 8)));
    }

    /// A C++ exception in the native code becomes a Java RuntimeException. Here the kernel doesn't match the image
    @Test
    public void nativeExceptionBecomesJava() {
        NativeImageConvolveNormalized alg = new NativeImageConvolveNormalized();
        alg.setImageType(GrayU8.class);
        Kernel1D_F32 kernel = new Kernel1D_F32(3, new float[]{1, 2, 1});
        assertThrows(RuntimeException.class, () -> alg.nativehorizontal(kernel, new GrayU8(10, 8), new GrayU8(10, 8)));
    }
}
