package org.boofcpp.testing;

import org.boofcpp.NativeConcurrency;
import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;

import static org.junit.jupiter.api.Assertions.assertEquals;

/// Tests run with one thread, and the thread count from before each test is restored after it
@ExtendWith(NativeSingleThreaded.class)
public class TestNativeSingleThreaded extends BoofCPPStandardJUnit {
    static int originalThreads;

    @BeforeAll static void setThreads() {
        originalThreads = NativeConcurrency.getMaxThreads();
        NativeConcurrency.setMaxThreads(3);
    }

    @AfterAll static void restored() {
        int threads = NativeConcurrency.getMaxThreads();
        NativeConcurrency.setMaxThreads(originalThreads);
        assertEquals(3, threads);
    }

    @Test void oneThread() { assertEquals(1, NativeConcurrency.getMaxThreads()); }
}
