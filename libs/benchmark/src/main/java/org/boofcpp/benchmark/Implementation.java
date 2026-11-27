package org.boofcpp.benchmark;

import boofcv.override.BOverrideManager;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.fast.BoofCPPFast;

import java.util.ArrayList;
import java.util.List;

/// What BoofCV's API runs: its own Java code, or the hooks of BoofCPP variants registered as a user would
public enum Implementation {
    /// BoofCV's Java code, without hooks
    java(null),
    /// Strict's hooks
    strict(java),
    /// Fast's hooks registered after strict's, which handle what fast doesn't have
    fast(strict);

    /// The implementation whose hooks are registered before this one's own, or null for java
    public final Implementation base;

    Implementation( Implementation base ) {
        this.base = base;
    }

    /// Registers the hooks this implementation runs with, its base's then its own, after removing every registered hook
    public void register() {
        BOverrideManager.clear();
        for (Implementation i : chain())
            i.registerOwn();
    }

    /// Registers only its own hooks, over those already registered
    public void registerOwn() {
        if (this == strict)
            BoofCPPStrict.initialize();
        else if (this == fast)
            BoofCPPFast.initialize();
    }

    /// Its bases and itself, the first base first
    private List<Implementation> chain() {
        var chain = new ArrayList<Implementation>();
        for (Implementation i = this; i != null; i = i.base)
            chain.add(0, i);
        return chain;
    }

    /// True if it runs on this machine
    public boolean isSupported() {
        return this != fast || BoofCPPFast.isLoaded();
    }
}
