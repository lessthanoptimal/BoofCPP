package org.boofcpp.convolve;

import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.alg.filter.derivative.GImageDerivativeOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.struct.border.BorderType;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.struct.border.ImageBorder;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.BoofTesting;
import org.boofcpp.BoofCPP;
import org.junit.jupiter.api.Test;

import java.util.Random;

public class TestNativeConvolveImage {
    Random rand = new Random(23423);

    Class types[] = new Class[]{GrayU8.class, GrayF32.class};

    BorderType borderTypes[] = new BorderType[]{BorderType.WRAP,BorderType.REFLECT,BorderType.EXTENDED,BorderType.ZERO};

    int width=200,height=150;

    static {
        BoofCPP.loadlib();
    }

    @Test
    public void compareHorizontal() {
        NativeConvolveImage nativeConv = new NativeConvolveImage();

        for( Class type : types ) {
            Class typeOutput = GImageDerivativeOps.getDerivativeType(type);
            ImageGray input = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray found = GeneralizedImageOps.createSingleBand(typeOutput,width,height);
            ImageGray expected = GeneralizedImageOps.createSingleBand(typeOutput,width,height);

            GImageMiscOps.fillUniform(input,rand,0,255);

            input = BoofTesting.createSubImageOf(input); // non-zero offset/stride


            for( int radius : new int[]{1,2,3,4,5,24} ) {
                for(BorderType borderType : borderTypes) {

                    ImageBorder border = FactoryImageBorder.single(borderType,type);

                    Kernel1D kernel = FactoryKernelGaussian.gaussian1D(type, -1, radius);

                    nativeConv.horizontal(kernel, input, found, border);
                    GConvolveImageOps.horizontal(kernel, input, expected,border);

                    BoofTesting.assertEquals(expected, found, 0); // bit identical to BoofCV
                }
            }
        }
    }

    @Test
    public void compareVertical() {
        NativeConvolveImage nativeConv = new NativeConvolveImage();

        for( Class type : types ) {
            Class typeOutput = GImageDerivativeOps.getDerivativeType(type);
            ImageGray input = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray found = GeneralizedImageOps.createSingleBand(typeOutput,width,height);
            ImageGray expected = GeneralizedImageOps.createSingleBand(typeOutput,width,height);

            GImageMiscOps.fillUniform(input,rand,0,255);

            input = BoofTesting.createSubImageOf(input); // non-zero offset/stride


            for( int radius : new int[]{1,2,3,4,5,24} ) {
                for(BorderType borderType : borderTypes) {

                    ImageBorder border = FactoryImageBorder.single(borderType,type);

                    Kernel1D kernel = FactoryKernelGaussian.gaussian1D(type, -1, radius);

                    nativeConv.vertical(kernel, input, found, border);
                    GConvolveImageOps.vertical(kernel, input, expected,border);

                    BoofTesting.assertEquals(expected, found, 0); // bit identical to BoofCV
                }
            }

        }
    }

    @Test
    public void compareConvolve() {
        NativeConvolveImage nativeConv = new NativeConvolveImage();

        // faster test
        int width = this.width/2;
        int height = this.height/2;

        for( Class type : types ) {
            Class typeOutput = GImageDerivativeOps.getDerivativeType(type);
            ImageGray input = GeneralizedImageOps.createSingleBand(type,width,height);
            ImageGray found = GeneralizedImageOps.createSingleBand(typeOutput,width,height);
            ImageGray expected = GeneralizedImageOps.createSingleBand(typeOutput,width,height);

            GImageMiscOps.fillUniform(input,rand,0,255);

            input = BoofTesting.createSubImageOf(input); // non-zero offset/stride


            for( int radius : new int[]{1,2,3,4,5,24} ) {
                for(BorderType borderType : borderTypes) {

                    ImageBorder border = FactoryImageBorder.single(borderType,type);

                    Kernel2D kernel = FactoryKernelGaussian.gaussian2D(type, -1, radius);

                    nativeConv.convolve(kernel, input, found, border);
                    GConvolveImageOps.convolve(kernel, input, expected,border);

                    BoofTesting.assertEquals(expected, found, 0); // bit identical to BoofCV
                }
            }

        }
    }
}
