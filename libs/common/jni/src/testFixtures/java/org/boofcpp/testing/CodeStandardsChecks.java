package org.boofcpp.testing;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

/// Checks of CodeStandards.md which each module runs on its own test code
public class CodeStandardsChecks {
    /// Every unit test class, a Test*.java file in 'testJava', extends BoofCPPStandardJUnit
    public static void assertTestsExtendStandard( File testJava ) throws IOException {
        assertTrue(testJava.isDirectory(), "Not a directory: " + testJava.getAbsolutePath());
        Path root = testJava.toPath();
        List<Path> files;
        try (Stream<Path> paths = Files.walk(root)) {
            files = paths.filter(p -> p.getFileName().toString().matches("Test[A-Z]\\w*\\.java"))
                    .collect(Collectors.toList());
        }

        List<String> failures = new ArrayList<>();
        for (Path file : files) {
            String name = root.relativize(file).toString().replace(File.separatorChar, '.').replace(".java", "");
            try {
                if (!BoofCPPStandardJUnit.class.isAssignableFrom(Class.forName(name)))
                    failures.add(name);
            } catch (ClassNotFoundException e) {
                failures.add(name + " (not found)");
            }
        }
        if (!failures.isEmpty())
            fail("Must extend BoofCPPStandardJUnit: " + String.join(", ", failures));
    }
}
