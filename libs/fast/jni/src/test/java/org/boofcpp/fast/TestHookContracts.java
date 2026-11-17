package org.boofcpp.fast;

import org.boofcpp.testing.HookContractChecks;

public class TestHookContracts extends HookContractChecks {
    @Override protected void registerHooks() {
        if (!BoofCPPFast.isLoaded())
            throw new RuntimeException("Fast's native library didn't load");
        new FastVariant().initialize();
    }
}
