package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.alg.filter.convolve.BOverrideConvolveImageNormalized;
import boofcv.alg.filter.convolve.GConvolveImageOps;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.filter.kernel.FactoryKernelGaussian;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel1D_F32;
import boofcv.struct.convolve.Kernel1D_F64;
import boofcv.struct.convolve.Kernel1D_S32;
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
    private final Class[] imageTypes;

    protected <H extends BOverrideConvolveImageNormalized.Horizontal & BOverrideConvolveImageNormalized.Vertical>
    NativeConvolveNormalizedChecks( H hook, Class... imageTypes ) {
        this.horizontal = hook;
        this.vertical = hook;
        this.imageTypes = imageTypes;
    }

    /// Gaussian kernels, also scaled so their sum isn't one, which makes BoofCV normalize them first
    @Test
    public void compareHorizontal() {
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

    @Test
    public void compareVertical() {
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

    /// Types the variant doesn't support throw a RuntimeException, so BoofCV uses its Java code
    @Test
    public void unsupportedTypesThrow() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 10, 8);
            Kernel1D kernel = new Kernel1D_S32(3, new int[]{1, 2, 1});
            assertThrows(RuntimeException.class, () -> horizontal.horizontal(kernel, input, input.createSameShape()));
            assertThrows(RuntimeException.class, () -> vertical.vertical(kernel, input, input.createSameShape()));
        }
    }

    /// A kernel whose type doesn't match the image throws a RuntimeException instead of crashing
    @Test
    public void mismatchedKernelThrows() {
        for (Class type : imageTypes) {
            ImageGray input = GeneralizedImageOps.createSingleBand(type, 10, 8);
            boolean isFloat = type == GrayF32.class || type == GrayF64.class;
            Kernel1D kernel = isFloat ? new Kernel1D_S32(3, new int[]{1, 2, 1}) :
                    new Kernel1D_F32(3, new float[]{1, 2, 1});
            assertThrows(RuntimeException.class, () -> horizontal.horizontal(kernel, input, input.createSameShape()));
            assertThrows(RuntimeException.class, () -> vertical.vertical(kernel, input, input.createSameShape()));
        }
    }

    private static Kernel1D scaled( Kernel1D kernel, int scale ) {
        for (int i = 0; i < kernel.width; i++)
            kernel.setD(i, kernel.getDouble(i)*scale);
        return kernel;
    }

    /// Random sub-image, so that its offset and stride aren't the defaults
    private ImageGray randomImage( Class type ) {
        ImageGray input = GeneralizedImageOps.createSingleBand(type, 400, 300);
        GImageMiscOps.fillUniform(input, rand, 0, MAX_PIXEL);
        return BoofTesting.createSubImageOf(input);
    }
}
