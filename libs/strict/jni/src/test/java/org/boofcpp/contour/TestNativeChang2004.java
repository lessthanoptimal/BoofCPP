package org.boofcpp.contour;

import org.boofcpp.BoofCPPStrict;
import org.boofcpp.testing.NativeChang2004Checks;

/// The Chang2004 contour checks shared with the other variants
public class TestNativeChang2004 extends NativeChang2004Checks {
    static {
        BoofCPPStrict.loadlib();
    }

    public TestNativeChang2004() {
        super(NativeChang2004::new);
    }
}
