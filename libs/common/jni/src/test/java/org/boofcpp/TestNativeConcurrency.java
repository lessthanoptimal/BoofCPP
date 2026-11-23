package org.boofcpp;

import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class TestNativeConcurrency extends BoofCPPStandardJUnit {
    final int originalThreads = NativeConcurrency.getMaxThreads();

    @AfterEach void cleanUp() {
        NativeConcurrency.setMaxThreads(originalThreads);
        NativeConcurrency.setTestingBlocks(0);
    }

    @Test void setMaxThreads() {
        assertTrue(NativeConcurrency.getMaxThreads() >= 1);
        NativeConcurrency.setMaxThreads(3);
        assertEquals(3, NativeConcurrency.getMaxThreads());
        NativeConcurrency.setMaxThreads(1);
        assertEquals(1, NativeConcurrency.getMaxThreads());
    }

    @Test void setMaxThreads_invalid() {
        NativeConcurrency.setMaxThreads(2);
        assertThrows(IllegalArgumentException.class, () -> NativeConcurrency.setMaxThreads(0));
        assertThrows(IllegalArgumentException.class, () -> NativeConcurrency.setMaxThreads(-1));
        assertEquals(2, NativeConcurrency.getMaxThreads());
    }

    @Test void setTestingBlocks() {
        assertEquals(0, NativeConcurrency.getTestingBlocks());
        NativeConcurrency.setTestingBlocks(7);
        assertEquals(7, NativeConcurrency.getTestingBlocks());
        assertThrows(IllegalArgumentException.class, () -> NativeConcurrency.setTestingBlocks(-1));
        assertEquals(7, NativeConcurrency.getTestingBlocks());
    }
}
