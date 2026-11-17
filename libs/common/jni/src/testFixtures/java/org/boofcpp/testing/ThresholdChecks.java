package org.boofcpp.testing;

import boofcv.BoofTesting;
import boofcv.abst.filter.binary.InputToBinary;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.override.BOverrideManager;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;

import java.util.function.Consumer;
import java.util.function.Supplier;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;

/// Base of the checks of a variant's thresholding filters, which BoofCV creates through BOverrideFactoryThresholdBinary.
/// The binary images must be identical to BoofCV's
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class ThresholdChecks extends BoofCPPStandardJUnit {
    protected final Class[] imageTypes;
    /// Registers the variant's hook with BoofCV
    private final Runnable register;

    protected ThresholdChecks( Runnable register, Class... imageTypes ) {
        this.register = register;
        this.imageTypes = imageTypes;
    }

    /// The variant's filter for 'type' with typical arguments
    protected abstract InputToBinary create( Class type );

    @AfterEach
    public void clearHooks() {
        BOverrideManager.clear();
    }

    /// Types the variant doesn't support return null or throw a RuntimeException, so BoofCV uses its own filter
    @Test
    public void unsupportedTypesDecline() {
        for (Class type : BoofCPPTesting.unsupported(imageTypes)) {
            InputToBinary filter;
            try {
                filter = create(type);
            } catch (RuntimeException ignore) {
                continue;
            }
            assertNull(filter, type.getSimpleName());
        }
    }

    /// Images of every supported type with values in [0, 255]: even and odd shapes, so blocks can be partial, whole
    /// images and sub-images
    protected void forEachInput( Consumer<ImageGray> check ) {
        for (int[] shape : new int[][]{{400, 300}, {401, 297}}) {
            for (boolean sub : new boolean[]{false, true}) {
                for (Class type : imageTypes) {
                    ImageGray input = GeneralizedImageOps.createSingleBand(type, shape[0], shape[1]);
                    GImageMiscOps.fillUniform(input, rand, 0, 255);
                    check.accept(sub ? BoofTesting.createSubImageOf(input) : input);
                }
            }
        }
    }

    /// Compares the variant's filter with BoofCV's, both called directly and created by BoofCV's factory with the
    /// variant's hook registered
    ///
    /// @param variant Creates the variant's filter
    /// @param factory Creates a filter with BoofCV's factory
    protected void compare( ImageGray input, Supplier<InputToBinary> variant, Supplier<InputToBinary> factory ) {
        BOverrideManager.clear();
        InputToBinary boofcv = factory.get();
        InputToBinary hooked;
        try {
            register.run();
            hooked = factory.get();
        } finally {
            BOverrideManager.clear();
        }
        InputToBinary direct = variant.get();
        assertEquals(direct.getClass(), hooked.getClass(), "BoofCV's factory didn't use the hook");

        GrayU8 expected = new GrayU8(input.width, input.height);
        boofcv.process(input, expected);
        for (InputToBinary filter : new InputToBinary[]{direct, hooked}) {
            GrayU8 found = new GrayU8(1, 1);
            filter.process(input, found);
            BoofCPPTesting.assertIdentical(expected, found);
        }
    }
}
