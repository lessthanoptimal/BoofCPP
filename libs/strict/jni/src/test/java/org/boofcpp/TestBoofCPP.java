package org.boofcpp;

import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class TestBoofCPP extends BoofCPPStandardJUnit {
    /// Names of the test variants, in the order they were initialized
    static final List<String> initialized = new ArrayList<>();

    @AfterEach void cleanUp() {
        BoofCPP.uninitialize();
        initialized.clear();
    }

    /// The test variants are listed in src/test/resources/META-INF/services. Supported ones are initialized after
    /// the strict hooks, in order of increasing priority
    @Test void initialize_variants() {
        BoofCPP.initialize();
        assertEquals(List.of("low", "high"), initialized);
    }

    @Test void variants() {
        List<NativeVariant> variants = BoofCPP.variants();
        assertEquals(2, variants.size());
        assertEquals(LowPriority.class, variants.get(0).getClass());
        assertEquals(HighPriority.class, variants.get(1).getClass());
    }

    public static class LowPriority extends Recording {
        public LowPriority() { super("low", 1, true); }
    }

    public static class HighPriority extends Recording {
        public HighPriority() { super("high", 5, true); }
    }

    public static class Unsupported extends Recording {
        public Unsupported() { super("unsupported", 3, false); }
    }

    /// Records that it was initialized instead of registering hooks
    static class Recording implements NativeVariant {
        final String name;
        final int priority;
        final boolean supported;

        Recording( String name, int priority, boolean supported ) {
            this.name = name;
            this.priority = priority;
            this.supported = supported;
        }

        @Override public int priority() { return priority; }

        @Override public boolean isSupported() { return supported; }

        @Override public void initialize() { initialized.add(name); }
    }
}
