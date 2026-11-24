package org.boofcpp.fast;

import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.boofcpp.testing.CodeStandardsChecks;
import org.junit.jupiter.api.Test;

import java.io.File;

/// Checks this module's code against CodeStandards.md
public class TestEnforceCodeStandards extends BoofCPPStandardJUnit {
    @Test public void unitTestsMustExtendStandard() throws Exception {
        CodeStandardsChecks.assertTestsExtendStandard(new File("src/test/java"));
    }
}
