package org.boofcpp.testing;

import boofcv.alg.filter.convolve.BOverrideConvolveImage;
import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.alg.filter.derivative.GImageDerivativeOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.factory.filter.kernel.FactoryKernel;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.convolve.Kernel1D_F32;
import boofcv.struct.convolve.Kernel1D_F64;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D_F32;
import boofcv.struct.convolve.Kernel2D_F64;
import boofcv.struct.convolve.Kernel2D_S32;
import boofcv.struct.convolve.KernelBase;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.ImageGray;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;

import static org.boofcpp.testing.BoofCPPTesting.singleThreaded;
import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's ConvolveImage hooks against BoofCV's single threaded Java code, shared by every variant. A
/// variant's test class extends this, passes its hook and the input image types it supports, and sets its tolerance.
/// Output images have BoofCV's derivative type, e.g. GrayS16 for GrayU8
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeConvolveImageChecks extends BoofCPPStandardJUnit {
    private static final BorderType[] BORDERS =
            {BorderType.WRAP, BorderType.REFLECT, BorderType.EXTENDED, BorderType.ZERO};
    private static final int[] RADII = {1, 2, 3, 4, 5, 24};
    private static final double MAX_PIXEL = 255;

    private final BOverrideConvolveImage.Horizontal horizontal;
    private final BOverrideConvolveImage.Vertical vertical;
    private final BOverrideConvolveImage.Convolve convolve;
    private final Class[] imageTypes;

    protected <H extends BOverrideConvolveImage.Horizontal & BOverrideConvolveImage.Vertical &
            BOverrideConvolveImage.Convolve> NativeConvolveImageChecks( H hook, Class... imageTypes ) {
        this.horizontal = hook;
        this.vertical = hook;
        this.convolve = hook;
        this.imageTypes = imageTypes;
    }

    @Test public void compareHorizontal() {
        for (Class type : imageTypes) {
            ImageGray input = randomSubImage(type, 200, 150, 0, MAX_PIXEL);
            for (Kernel1D kernel : kernels1D(type)) {
                for (BorderType borderType : BORDERS) {
                    ImageBorder border = FactoryImageBorder.single(borderType, type);
                    ImageGray expected = createOutput(input), found = createOutput(input);
                    singleThreaded(() -> GConvolveImageOps.horizontal(kernel, input, expected, border));
                    horizontal.horizontal(kernel, input, found, border);
                    assertMatches(expected, found, scale(kernel));
                }
            }
        }
    }

    @Test public void compareVertical() {
        for (Class type : imageTypes) {
            ImageGray input = randomSubImage(type, 200, 150, 0, MAX_PIXEL);
            for (Kernel1D kernel : kernels1D(type)) {
                for (BorderType borderType : BORDERS) {
                    ImageBorder border = FactoryImageBorder.single(borderType, type);
                    ImageGray expected = createOutput(input), found = createOutput(input);
                    singleThreaded(() -> GConvolveImageOps.vertical(kernel, input, expected, border));
                    vertical.vertical(kernel, input, found, border);
                    assertMatches(expected, found, scale(kernel));
                }
            }
        }
    }

    @Test public void compareConvolve() {
        for (Class type : imageTypes) {
            ImageGray input = randomSubImage(type, 100, 75, 0, MAX_PIXEL);
            for (Kernel2D kernel : kernels2D(type)) {
                for (BorderType borderType : BORDERS) {
                    ImageBorder border = FactoryImageBorder.single(borderType, type);
                    ImageGray expected = createOutput(input), found = createOutput(input);
                    singleThreaded(() -> GConvolveImageOps.convolve(kernel, input, expected, border));
                    convolve.convolve(kernel, input, found, border);
                    assertMatches(expected, found, scale(kernel));
                }
            }
        }
    }

    /// Input types the variant doesn't support throw a RuntimeException, so BoofCV uses its Java code
    @Test public void unsupportedTypesThrow() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 10, 8);
            ImageGray output = (ImageGray)input.createSameShape();
            ImageBorder border = FactoryImageBorder.single(BorderType.EXTENDED, type);
            Kernel1D kernel1;
            Kernel2D kernel2;
            if (type == GrayF32.class) {
                kernel1 = new Kernel1D_F32(3, new float[]{1, 2, 1});
                kernel2 = new Kernel2D_F32(3, new float[]{1, 2, 1, 2, 4, 2, 1, 2, 1});
            } else if (type == GrayF64.class) {
                kernel1 = new Kernel1D_F64(3, new double[]{1, 2, 1});
                kernel2 = new Kernel2D_F64(3, new double[]{1, 2, 1, 2, 4, 2, 1, 2, 1});
            } else {
                kernel1 = new Kernel1D_S32(3, new int[]{1, 2, 1});
                kernel2 = new Kernel2D_S32(3, new int[]{1, 2, 1, 2, 4, 2, 1, 2, 1});
            }
            assertThrows(RuntimeException.class, () -> horizontal.horizontal(kernel1, input, output, border));
            assertThrows(RuntimeException.class, () -> vertical.vertical(kernel1, input, output, border));
            assertThrows(RuntimeException.class, () -> convolve.convolve(kernel2, input, output, border));
        }
    }

    /// Gaussian kernels, and random ones with negative weights, which make floats cancel
    private List<Kernel1D> kernels1D( Class type ) {
        List<Kernel1D> kernels = new ArrayList<>();
        for (int radius : RADII) {
            kernels.add(FactoryKernelGaussian.gaussian1D(type, -1, radius));
            kernels.add(FactoryKernel.random(FactoryKernel.getKernelType(type, 1), radius, -10, 10, rand));
        }
        return kernels;
    }

    private List<Kernel2D> kernels2D( Class type ) {
        List<Kernel2D> kernels = new ArrayList<>();
        for (int radius : RADII) {
            kernels.add(FactoryKernelGaussian.gaussian2D(type, -1, radius));
            kernels.add(FactoryKernel.random(FactoryKernel.getKernelType(type, 2), radius, -10, 10, rand));
        }
        return kernels;
    }

    /// Tolerance scale: rounding error of a sum grows with the sum of its terms' magnitudes, at most
    /// sum(|weight|)*max(|pixel|)
    private static double scale( KernelBase kernel ) {
        double sum = 0;
        if (kernel instanceof Kernel1D) {
            for (int i = 0; i < kernel.width; i++)
                sum += Math.abs(((Kernel1D)kernel).getDouble(i));
        } else {
            for (int y = 0; y < kernel.width; y++)
                for (int x = 0; x < kernel.width; x++)
                    sum += Math.abs(((Kernel2D)kernel).getDouble(x, y));
        }
        return sum*MAX_PIXEL;
    }

    private static ImageGray createOutput( ImageGray input ) {
        Class outputType = GImageDerivativeOps.getDerivativeType(input.getClass());
        return GeneralizedImageOps.createSingleBand(outputType, input.width, input.height);
    }
}
