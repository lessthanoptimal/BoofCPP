package org.boofcpp;

/// A variant of BoofCPP, e.g. fast, which replaces some of the strict hooks with its own. `BoofCPP.initialize()`
/// finds variants with [java.util.ServiceLoader]: a variant's jar lists its implementation in
/// `META-INF/services/org.boofcpp.NativeVariant`.
public interface NativeVariant {
    /// Variants are initialized in order of increasing priority, so a higher priority replaces the hooks of a lower one
    int priority();

    /// True if its native library is loaded and it can run on this CPU
    boolean isSupported();

    /// Registers its hooks with BoofCV
    void initialize();
}
