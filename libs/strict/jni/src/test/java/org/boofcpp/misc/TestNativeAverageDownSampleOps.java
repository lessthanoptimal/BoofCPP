package org.boofcpp.misc;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayF64;
import boofcv.struct.image.GrayS16;
import boofcv.struct.image.GrayS32;
import boofcv.struct.image.GrayS8;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.testing.NativeAverageDownSampleChecks;

/// The average down sampling checks shared with the other variants. Strict supports every type BoofCV does
public class TestNativeAverageDownSampleOps extends NativeAverageDownSampleChecks {
    static {
        BoofCPPStrict.loadlib();
    }

    public TestNativeAverageDownSampleOps() {
        super(new NativeAverageDownSampleOps(), GrayU8.class, GrayS8.class, GrayU16.class, GrayS16.class, GrayS32.class,
                GrayF32.class, GrayF64.class);
    }
}
