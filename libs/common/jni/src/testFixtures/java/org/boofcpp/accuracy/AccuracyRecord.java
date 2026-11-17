package org.boofcpp.accuracy;

import boofcv.BoofVersion;
import boofcv.concurrency.BoofConcurrency;
import org.boofcpp.BoofCPPVersion;
import org.boofcpp.testing.BenchmarkUtils;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.time.Instant;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/// A variant's accuracy against BoofCV's Java code, and what it was measured on. Saved as JSON in
/// `results/accuracy/<variant>/<platform>.json`. Metrics which grew more than [#GROWTH] times since the saved record
/// are reported.
public class AccuracyRecord {
    /// A metric which grew more than this many times is reported
    public static final double GROWTH = 2.0;

    /// Metrics in the order they're saved
    static final String[] METRICS = {"max_abs", "max_rel", "rms", "max_ulp", "differing"};

    public final Map<String, String> info = new LinkedHashMap<>();
    /// Metrics of each result, keyed by "op|image|case"
    public final Map<String, double[]> results = new LinkedHashMap<>();

    /// Runs the cases and saves the record, then prints the metrics that grew since the record it replaced
    public static void run( String variant, String nativeBuildInfo, AccuracyCase... cases ) throws IOException {
        // The reference is single-threaded Java
        BoofConcurrency.USE_CONCURRENT = false;

        AccuracyRecord record = new AccuracyRecord();
        record.info.put("variant", variant);
        record.info.put("platform", platform());
        record.info.put("cpu", cpuName());
        record.info.put("native", nativeBuildInfo);
        record.info.put("java", System.getProperty("java.vendor") + " " + System.getProperty("java.version"));
        record.info.put("boofcpp", BoofCPPVersion.VERSION + " " + BoofCPPVersion.GIT_SHA +
                (BoofCPPVersion.DIRTY != 0 ? " dirty" : ""));
        record.info.put("boofcv", BoofVersion.VERSION + " " + BoofVersion.GIT_SHA);
        record.info.put("date", Instant.now().toString());
        for (AccuracyCase c : cases) {
            c.run(record);
        }

        File file = new File(BenchmarkUtils.findProjectRoot(),
                "results/accuracy/" + variant + "/" + record.info.get("platform") + ".json");
        List<String> grew = file.exists() ? record.grewSince(read(file)) : new ArrayList<>();
        file.getParentFile().mkdirs();
        Files.write(file.toPath(), record.toJson().getBytes(StandardCharsets.UTF_8));

        System.out.println(record.table());
        System.out.println("Saved to " + file.getPath());
        if (grew.isEmpty()) {
            System.out.println("No metric grew more than " + GROWTH + "x");
        } else {
            System.out.println("Grew more than " + GROWTH + "x:");
            grew.forEach(s -> System.out.println("  " + s));
        }
    }

    /// Adds a result. Names can't contain '|'
    public void add( String op, String image, String name, ImageDifference d ) {
        if ((op + image + name).contains("|"))
            throw new IllegalArgumentException("Names can't contain '|'");
        results.put(op + "|" + image + "|" + name, new double[]{d.maxAbs, d.maxRel, d.rms, d.maxUlp, d.differing});
    }

    /// Metrics which grew more than GROWTH times, or from zero, since 'previous'
    public List<String> grewSince( AccuracyRecord previous ) {
        List<String> grew = new ArrayList<>();
        for (Map.Entry<String, double[]> e : results.entrySet()) {
            double[] before = previous.results.get(e.getKey());
            if (before == null)
                continue;
            for (int i = 0; i < METRICS.length; i++) {
                double now = e.getValue()[i];
                if (now > GROWTH*before[i] || (before[i] == 0 && now > 0))
                    grew.add(e.getKey().replace('|', ' ') + " " + METRICS[i] + ": " + before[i] + " -> " + now);
            }
        }
        return grew;
    }

    /// One result per line, so that diffs of the saved records are readable
    public String toJson() {
        StringBuilder sb = new StringBuilder("{\n");
        for (Map.Entry<String, String> e : info.entrySet()) {
            sb.append("  \"").append(e.getKey()).append("\": \"").append(escape(e.getValue())).append("\",\n");
        }
        sb.append("  \"results\": [\n");
        int n = 0;
        for (Map.Entry<String, double[]> e : results.entrySet()) {
            String[] key = e.getKey().split("\\|", 3);
            sb.append(String.format(Locale.US, "    {\"op\": \"%s\", \"image\": \"%s\", \"case\": \"%s\"",
                    escape(key[0]), escape(key[1]), escape(key[2])));
            for (int i = 0; i < METRICS.length; i++) {
                sb.append(String.format(Locale.US, ", \"%s\": %.6g", METRICS[i], e.getValue()[i]));
            }
            sb.append(++n < results.size() ? "},\n" : "}\n");
        }
        return sb.append("  ]\n}\n").toString();
    }

    /// Reads a record saved by toJson()
    public static AccuracyRecord read( File file ) throws IOException {
        String json = new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8);
        AccuracyRecord record = new AccuracyRecord();
        Matcher info = Pattern.compile("^  \"(\\w+)\": \"((?:[^\"\\\\]|\\\\.)*)\",$", Pattern.MULTILINE).matcher(json);
        while (info.find()) {
            record.info.put(info.group(1), unescape(info.group(2)));
        }
        Matcher result = Pattern.compile(
                "\\{\"op\": \"((?:[^\"\\\\]|\\\\.)*)\", \"image\": \"((?:[^\"\\\\]|\\\\.)*)\", " +
                        "\"case\": \"((?:[^\"\\\\]|\\\\.)*)\"((?:, \"\\w+\": [^,}]+)*)\\}").matcher(json);
        Pattern metric = Pattern.compile("\"(\\w+)\": ([^,}]+)");
        while (result.find()) {
            double[] values = new double[METRICS.length];
            Matcher m = metric.matcher(result.group(4));
            while (m.find()) {
                for (int i = 0; i < METRICS.length; i++) {
                    if (METRICS[i].equals(m.group(1)))
                        values[i] = Double.parseDouble(m.group(2));
                }
            }
            record.results.put(unescape(result.group(1)) + "|" + unescape(result.group(2)) + "|" +
                    unescape(result.group(3)), values);
        }
        return record;
    }

    /// The metrics as a table
    public String table() {
        StringBuilder sb = new StringBuilder(String.format(Locale.US, "%-34s", "op image case"));
        for (String m : METRICS) {
            sb.append(String.format(Locale.US, " %11s", m));
        }
        for (Map.Entry<String, double[]> e : results.entrySet()) {
            sb.append(String.format(Locale.US, "\n%-34s", e.getKey().replace('|', ' ')));
            for (double v : e.getValue()) {
                sb.append(String.format(Locale.US, " %11.3g", v));
            }
        }
        return sb.toString();
    }

    /// Classifier of the platform, e.g. linux-amd64, the same as the native jar's
    static String platform() {
        String os = System.getProperty("os.name").toLowerCase(Locale.US);
        String arch = System.getProperty("os.arch").toLowerCase(Locale.US);
        if (os.contains("linux"))
            return "linux-" + arch;
        if (os.contains("mac"))
            return "macos-" + arch;
        if (os.contains("windows"))
            return "windows-" + arch;
        return "unknown-" + arch;
    }

    /// The CPU's model name on Linux. Elsewhere its architecture
    static String cpuName() {
        try {
            for (String line : Files.readAllLines(new File("/proc/cpuinfo").toPath())) {
                if (line.startsWith("model name") || line.startsWith("Model"))
                    return line.substring(line.indexOf(':') + 1).trim();
            }
        } catch (IOException ignore) {}
        return System.getProperty("os.arch");
    }

    static String escape( String s ) {
        return s.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    static String unescape( String s ) {
        return s.replace("\\\"", "\"").replace("\\\\", "\\");
    }
}
