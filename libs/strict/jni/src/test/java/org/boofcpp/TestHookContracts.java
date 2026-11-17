package org.boofcpp;

import org.boofcpp.testing.HookContractChecks;

public class TestHookContracts extends HookContractChecks {
    @Override protected void registerHooks() {
        BoofCPP.initialize();
    }
}
