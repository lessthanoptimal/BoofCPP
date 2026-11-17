package org.boofcpp.accuracy;

import org.boofcpp.testing.BoofCPPStandardJUnit;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

public class TestAccuracyRecord extends BoofCPPStandardJUnit {
    /// What's saved is read back unchanged
    @Test
    public void jsonRoundTrip( @TempDir File dir ) throws Exception {
        AccuracyRecord record = new AccuracyRecord();
        record.info.put("variant", "fast");
        record.info.put("native", "GCC \"13\" \\ -O3");
        record.add("mean", "GrayF32", "test_image r=5", difference(1.5e-5, 0.25));
        record.add("mean", "GrayF64", "large, values", difference(0, 0));

        File file = new File(dir, "record.json");
        Files.write(file.toPath(), record.toJson().getBytes(StandardCharsets.UTF_8));
        AccuracyRecord found = AccuracyRecord.read(file);

        assertEquals(record.info, found.info);
        assertEquals(record.results.keySet(), found.results.keySet());
        for (String key : record.results.keySet()) {
            assertArrayEquals(record.results.get(key), found.results.get(key), 1e-10);
        }
    }

    @Test
    public void add_badName() {
        assertThrows(IllegalArgumentException.class,
                () -> new AccuracyRecord().add("mean", "GrayF32", "a|b", difference(0, 0)));
    }

    /// Growth more than GROWTH times, or from zero, is reported. Results missing from either record are ignored
    @Test
    public void grewSince() {
        AccuracyRecord before = new AccuracyRecord();
        before.add("mean", "GrayF32", "a", difference(1e-5, 0.5));
        before.add("mean", "GrayF32", "b", difference(0, 0));
        before.add("mean", "GrayF32", "removed", difference(0, 0));

        AccuracyRecord now = new AccuracyRecord();
        now.add("mean", "GrayF32", "a", difference(2e-5, 1.1));
        now.add("mean", "GrayF32", "b", difference(1e-9, 0));
        now.add("mean", "GrayF32", "new", difference(1, 1));

        List<String> grew = now.grewSince(before);
        assertEquals(2, grew.size());
        assertTrue(grew.get(0).startsWith("mean GrayF32 a differing"));
        assertTrue(grew.get(1).startsWith("mean GrayF32 b max_abs"));
    }

    private static ImageDifference difference( double maxAbs, double differing ) {
        ImageDifference d = new ImageDifference();
        d.maxAbs = maxAbs;
        d.differing = differing;
        return d;
    }
}
