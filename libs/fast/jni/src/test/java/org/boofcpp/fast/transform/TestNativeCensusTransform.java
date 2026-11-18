package org.boofcpp.fast.transform;

import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU16;
import boofcv.struct.image.GrayU8;
import org.boofcpp.fast.BoofCPPFast;
import org.boofcpp.testing.NativeCensusChecks;

/// The census transform checks shared with the other variants. Census images are integers, so fast's results are
/// identical
public class TestNativeCensusTransform extends NativeCensusChecks {
    static {
        BoofCPPFast.isLoaded();
    }

    public TestNativeCensusTransform() {
        super(new NativeCensusTransform<>(null), GrayU8.class, GrayU16.class, GrayF32.class);
    }
}
