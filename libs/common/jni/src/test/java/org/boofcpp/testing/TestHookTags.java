package org.boofcpp.testing;

import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

public class TestHookTags extends BoofCPPStandardJUnit {
    static String source( String... lines ) {
        return String.join("\n", lines) + "\n";
    }

    @Test void overrides() {
        HookTags tags = HookTags.parse(source(
                "package a.b;",
                "",
                "import x.Y;",
                "",
                "/// Some class",
                "///",
                "/// BOOFCPP_OVERRIDE BOverrideBlurImageOps.mean",
                "/// BOOFCPP_OVERRIDE BOverrideBlurImageOps.gaussian",
                "@SuppressWarnings(\"rawtypes\")",
                "public class Foo<T extends Y<T>> implements BOverrideBlurImageOps.Mean<T>,",
                "        BOverrideBlurImageOps.Gaussian<T>, Runnable {",
                "    /// BOOFCPP_OVERRIDE BOverrideConvolveImage.horizontal",
                "    class Inner {}",
                "}"));
        assertEquals("a.b.Foo", tags.className);
        assertEquals(List.of("BOverrideBlurImageOps.mean", "BOverrideBlurImageOps.gaussian"), tags.overrides);
        assertEquals(List.of("BOverrideBlurImageOps.Mean", "BOverrideBlurImageOps.Gaussian"), tags.interfaces);
        assertFalse(tags.ignore);
    }

    @Test void ignore() {
        HookTags tags = HookTags.parse(source(
                "package a;",
                "",
                "/// Slower than Java",
                "/// BOOFCPP_IGNORE true",
                "public class Foo implements BOverrideBlurImageOps.Mean {}"));
        assertTrue(tags.ignore);
        assertTrue(tags.overrides.isEmpty());
        assertEquals(List.of("BOverrideBlurImageOps.Mean"), tags.interfaces);
    }

    /// Tags must be in the class Javadoc, right before the declaration
    @Test void onlyClassJavadoc() {
        HookTags tags = HookTags.parse(source(
                "package a;",
                "",
                "/// BOOFCPP_OVERRIDE BOverrideBlurImageOps.mean",
                "",
                "import x.Y;",
                "",
                "public class Foo {}"));
        assertTrue(tags.overrides.isEmpty());
        assertTrue(tags.interfaces.isEmpty());
    }

    @Test void unknownTagThrows() {
        assertThrows(IllegalArgumentException.class, () -> HookTags.parse(source(
                "package a;",
                "",
                "/// BOOFCPP_OVERIDE BOverrideBlurImageOps.mean",
                "public class Foo {}")));
    }

    @Test void noClass() {
        assertNull(HookTags.parse("package a;\n"));
    }
}
