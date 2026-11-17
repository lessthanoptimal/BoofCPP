package org.boofcpp.transform;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.BoofCPP;
import org.boofcpp.testing.NativeCensusChecks;

/// The census transform checks shared with the other variants
public class TestNativeCensusTransform extends NativeCensusChecks {
    static {
        BoofCPP.loadlib();
    }

    public TestNativeCensusTransform() {
        super(new NativeCensusTransform<>(), GrayU8.class, GrayU16.class, GrayF32.class);
    }
}
