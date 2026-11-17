package org.boofcpp;

import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.ConfigDisparityError;
import boofcv.struct.ConfigLength;

/// Access to the C++ configurations, only for tests. Makes sure the C++ and Java configurations have the same fields
/// and defaults.
public class NativeConfigs {
    static {
        if (!NativeLoader.load("JNIBoofCPPCommonTest", "boofcpp-common-test"))
            throw new RuntimeException("The test library JNIBoofCPPCommonTest didn't load. Build the native code first");
    }

    /// Java names of every field in the C++ configuration
    public static native String[] fieldNamesBM();

    public static native String[] fieldNamesCensus();

    public static native String[] fieldNamesNCC();

    public static native String[] fieldNamesLength();

    /// Names of the C++ enum values, in order
    public static native String[] disparityErrorNames();

    public static native String[] borderTypeNames();

    public static native String[] censusVariantsNames();

    /// Copies 'src' into a C++ configuration and then from C++ into 'dst'
    public static native void roundTripBM( ConfigDisparityBM src, ConfigDisparityBM dst );

    public static native void roundTripCensus( ConfigDisparityError.Census src, ConfigDisparityError.Census dst );

    public static native void roundTripNCC( ConfigDisparityError.NCC src, ConfigDisparityError.NCC dst );

    public static native void roundTripLength( ConfigLength src, ConfigLength dst );

    /// Writes the C++ default values into 'dst'
    public static native void defaultsBM( ConfigDisparityBM dst );

    public static native void defaultsCensus( ConfigDisparityError.Census dst );

    public static native void defaultsNCC( ConfigDisparityError.NCC dst );

    public static native void defaultsLength( ConfigLength dst );
}
