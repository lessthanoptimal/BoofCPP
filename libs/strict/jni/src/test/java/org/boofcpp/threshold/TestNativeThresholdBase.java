package org.boofcpp.threshold;

import boofcv.abst.filter.binary.InputToBinary;
import boofcv.struct.ConfigLength;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayS8;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.Test;

import java.util.function.Function;

import static org.junit.jupiter.api.Assertions.*;

public class TestNativeThresholdBase extends BoofCPPStandardJUnit {
    static {
        BoofCPP.loadlib();
    }

    /// Each filter is created for the types native supports, and throws an IllegalArgumentException for others
    @Test
    public void supportedTypesOnly() {
        ConfigLength width = ConfigLength.fixed(5);
        checkTypes(type -> new NativeGlobalFixed(10, true, type));
        checkTypes(type -> new NativeGlobalOtsu(0, 255, 1.0, true, type));
        checkTypes(type -> new NativeBlockMean(width, 1.0, true, true, type));
        checkTypes(type -> new NativeBlockMinMax(width, 1.0, true, 2.0, true, type));
        checkTypes(type -> new NativeBlockOtsu(false, width, 0.1, 1.0, true, true, type));
        checkTypes(type -> new NativeLocalMean(width, 1.0, true, type));
    }

    @SuppressWarnings("rawtypes")
    private void checkTypes( Function<Class, InputToBinary> create ) {
        assertNotNull(create.apply(GrayU8.class));
        assertNotNull(create.apply(GrayF32.class));
        assertThrows(IllegalArgumentException.class, () -> create.apply(GrayS8.class));
        assertThrows(IllegalArgumentException.class, () -> create.apply(GrayS16.class));
        assertThrows(IllegalArgumentException.class, () -> create.apply(GrayF64.class));
    }
}
