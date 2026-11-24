package org.boofcpp.testing;

import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel1D_F32;
import boofcv.struct.convolve.Kernel1D_F64;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.convolve.Kernel2D_F32;
import boofcv.struct.convolve.Kernel2D_F64;
import boofcv.struct.convolve.Kernel2D_S32;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.ImageGray;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertThrows;

/// Checks of a variant's ConvolveImageNormalized hooks against BoofCV's Java code, shared by every variant. A
/// variant's test class extends this, passes its hook and the image types it supports, and sets its tolerance
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class NativeConvolveNormalizedChecks extends BoofCPPStandardJUnit {
    private static final int[] RADII = {1, 2, 3, 4, 5, 24};
    /// Pixels are in [0, MAX_PIXEL]. The kernels have positive weights and are normalized, so each output pixel is a
    /// weighted average and its rounding error grows with MAX_PIXEL, the tolerance's scale
    private static final double MAX_PIXEL = 255;

    private final BOverrideConvolveImageNormalized.Horizontal horizontal;
    private final BOverrideConvolveImageNormalized.Vertical vertical;
    private final BOverrideConvolveImageNormalized.Convolve convolve;
    private final Class[] imageTypes;

    protected <H extends BOverrideConvolveImageNormalized.Horizontal & BOverrideConvolveImageNormalized.Vertical &
            BOverrideConvolveImageNormalized.Convolve> NativeConvolveNormalizedChecks( H hook, Class... imageTypes ) {
        this.horizontal = hook;
        this.vertical = hook;
        this.convolve = hook;
        this.imageTypes = imageTypes;
    }

    /// Gaussian kernels, also scaled so their sum isn't one, which makes BoofCV normalize them first
    @Test public void compareHorizontal() {
        for (Class type : imageTypes) {
            ImageGray input = randomImage(type);
            for (int radius : RADII) {
                for (int scale : new int[]{1, 2}) {
                    Kernel1D kernel = scaled(FactoryKernelGaussian.gaussian1D(type, -1, radius), scale);
                    ImageGray expected = (ImageGray)input.createSameShape(), found = (ImageGray)input.createSameShape();
                    GConvolveImageOps.horizontalNormalized(kernel, input, expected);
                    horizontal.horizontal(kernel, input, found);
                    assertMatches(expected, found, MAX_PIXEL);
                }
            }
        }
    }

    @Test public void compareVertical() {
        for (Class type : imageTypes) {
            ImageGray input = randomImage(type);
            for (int radius : RADII) {
                for (int scale : new int[]{1, 2}) {
                    Kernel1D kernel = scaled(FactoryKernelGaussian.gaussian1D(type, -1, radius), scale);
                    ImageGray expected = (ImageGray)input.createSameShape(), found = (ImageGray)input.createSameShape();
                    GConvolveImageOps.verticalNormalized(kernel, input, expected);
                    vertical.vertical(kernel, input, found);
                    assertMatches(expected, found, MAX_PIXEL);
                }
            }
        }
    }

    /// 2D Gaussian kernels, also scaled, called directly and through BoofCV's hook. The widest kernel is wider than the
    /// small image, and as tall as the wide one, which BoofCV processes with its naive code
    @Test public void compareConvolve() {
        for (Class type : imageTypes) {
            for (ImageGray input : new ImageGray[]{randomImage(type), randomSubImage(type, 40, 30, 0, MAX_PIXEL),
                    randomSubImage(type, 60, 30, 0, MAX_PIXEL)}) {
                for (int radius : RADII) {
                    for (int scale : new int[]{1, 2}) {
                        Kernel2D kernel = scaled(FactoryKernelGaussian.gaussian2D(type, -1, radius), scale);
                        ImageGray expected = (ImageGray)input.createSameShape();
                        ImageGray found = (ImageGray)input.createSameShape();
                        BOverrideConvolveImageNormalized.convolve = null;
                        GConvolveImageOps.convolveNormalized(kernel, input, expected);
                        convolve.convolve(kernel, input, found);
                        assertMatches(expected, found, MAX_PIXEL);

                        ImageGray hooked = (ImageGray)input.createSameShape();
                        try {
                            BOverrideConvolveImageNormalized.convolve = convolve;
                            GConvolveImageOps.convolveNormalized(kernel, input, hooked);
                        } finally {
                            BOverrideConvolveImageNormalized.convolve = null;
                        }
                        assertMatches(expected, hooked, MAX_PIXEL);
                    }
                }
            }
        }
    }

    /// Types the variant doesn't support throw a RuntimeException, so BoofCV uses its Java code
    @Test public void unsupportedTypesThrow() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 10, 8);
            Kernel1D kernel = new Kernel1D_S32(3, new int[]{1, 2, 1});
            assertThrows(RuntimeException.class, () -> horizontal.horizontal(kernel, input, input.createSameShape()));
            assertThrows(RuntimeException.class, () -> vertical.vertical(kernel, input, input.createSameShape()));
            Kernel2D kernel2 = new Kernel2D_S32(3, new int[]{1, 2, 1, 2, 4, 2, 1, 2, 1});
            assertThrows(RuntimeException.class, () -> convolve.convolve(kernel2, input, input.createSameShape()));
        }
    }

    /// A kernel whose type doesn't match the image throws a RuntimeException instead of crashing
    @Test public void mismatchedKernelThrows() {
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 10, 8);
            boolean isFloat = type == GrayF32.class || type == GrayF64.class;
            Kernel1D kernel = isFloat ? new Kernel1D_S32(3, new int[]{1, 2, 1}) :
                    new Kernel1D_F32(3, new float[]{1, 2, 1});
            Kernel2D kernel2 = isFloat ? new Kernel2D_S32(3, new int[]{1, 2, 1, 2, 4, 2, 1, 2, 1}) :
                    new Kernel2D_F32(3, new float[]{1, 2, 1, 2, 4, 2, 1, 2, 1});
            assertThrows(RuntimeException.class, () -> horizontal.horizontal(kernel, input, input.createSameShape()));
            assertThrows(RuntimeException.class, () -> vertical.vertical(kernel, input, input.createSameShape()));
            assertThrows(RuntimeException.class, () -> convolve.convolve(kernel2, input, input.createSameShape()));
        }
    }

    private static Kernel1D scaled( Kernel1D kernel, int scale ) {
        for (int i = 0; i < kernel.width; i++)
            kernel.setD(i, kernel.getDouble(i)*scale);
        return kernel;
    }

    private static Kernel2D scaled( Kernel2D kernel, int scale ) {
        for (int y = 0; y < kernel.width; y++) {
            for (int x = 0; x < kernel.width; x++) {
                if (kernel instanceof Kernel2D_S32)
                    ((Kernel2D_S32)kernel).set(x, y, ((Kernel2D_S32)kernel).get(x, y)*scale);
                else if (kernel instanceof Kernel2D_F32)
                    ((Kernel2D_F32)kernel).set(x, y, ((Kernel2D_F32)kernel).get(x, y)*scale);
                else
                    ((Kernel2D_F64)kernel).set(x, y, ((Kernel2D_F64)kernel).get(x, y)*scale);
            }
        }
        return kernel;
    }

    private ImageGray randomImage( Class type ) {
        return randomSubImage(type, 400, 300, 0, MAX_PIXEL);
    }
}
