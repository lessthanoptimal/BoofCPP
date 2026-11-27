package org.boofcpp.fast;

import org.boofcpp.testing.HookContractChecks;

public class TestHookContracts extends HookContractChecks {
    @Override protected void registerHooks() {
        BoofCPPFast.initialize();
    }
}
