package org.boofcpp.benchmark;

import boofcv.BoofVersion;
import org.boofcpp.BoofCPPStrict;
import org.boofcpp.BoofCPPVersion;
import org.boofcpp.fast.BoofCPPFast;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/// What an implementation's saved results were measured with: when, on which machine and JDK, and with which builds of
/// BoofCV and BoofCPP. Written as `<impl>.metadata.json` next to `<impl>.json`
public class RunMetadata {
    /// The metadata of a run of 'implementation' on this machine, now
    public static Map<String, Object> of( Implementation implementation ) {
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("implementation", implementation.name());
        m.put("date", ZonedDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm z")));
        m.put("cpu", cpuModel());
        m.put("cores", Runtime.getRuntime().availableProcessors());
        m.put("os", System.getProperty("os.name") + " " + System.getProperty("os.version") + " " +
                System.getProperty("os.arch"));
        m.put("jdk", System.getProperty("java.version"));
        m.put("vm", System.getProperty("java.vm.name") + " " + System.getProperty("java.vm.version"));
        m.put("boofcv", version(BoofVersion.VERSION, BoofVersion.GIT_SHA, BoofVersion.DIRTY));
        if (implementation != Implementation.java) {
            m.put("boofcpp", version(BoofCPPVersion.VERSION, BoofCPPVersion.GIT_SHA, BoofCPPVersion.DIRTY));
            List<String> builds = new ArrayList<>();
            builds.add("strict: " + BoofCPPStrict.nativeBuildInfo());
            if (implementation == Implementation.fast)
                builds.add("fast: " + BoofCPPFast.nativeBuildInfo());
            m.put("native", String.join("; ", builds));
        }
        return m;
    }

    private static Map<String, Object> version( String version, String sha, int dirty ) {
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("version", version);
        m.put("sha", sha);
        m.put("dirty", dirty != 0);
        return m;
    }

    /// The CPU's model name, or the architecture if it can't be found
    static String cpuModel() {
        try {
            File info = new File("/proc/cpuinfo");
            if (info.exists()) {
                for (String line : Files.readAllLines(info.toPath())) {
                    if (line.startsWith("model name") || line.startsWith("Model"))
                        return line.substring(line.indexOf(':') + 1).trim();
                }
            }
            if (System.getProperty("os.name").toLowerCase().contains("mac"))
                return run("sysctl", "-n", "machdep.cpu.brand_string");
            String windows = System.getenv("PROCESSOR_IDENTIFIER");
            if (windows != null)
                return windows;
        } catch (IOException ignore) {}
        return System.getProperty("os.arch");
    }

    private static String run( String... command ) throws IOException {
        Process process = new ProcessBuilder(command).redirectErrorStream(true).start();
        try (var reader = new BufferedReader(new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
            String line = reader.readLine();
            return line == null ? System.getProperty("os.arch") : line.trim();
        }
    }

    /// 'value' as JSON. Supports maps, strings, numbers and booleans
    public static String toJson( Object value ) {
        if (value instanceof Map) {
            var out = new StringBuilder("{");
            for (Map.Entry<?, ?> e : ((Map<?, ?>)value).entrySet()) {
                if (out.length() > 1)
                    out.append(", ");
                out.append(toJson(e.getKey().toString())).append(": ").append(toJson(e.getValue()));
            }
            return out.append("}").toString();
        }
        if (value instanceof Number || value instanceof Boolean)
            return value.toString();
        var out = new StringBuilder("\"");
        for (char c : value.toString().toCharArray()) {
            switch (c) {
                case '"': out.append("\\\""); break;
                case '\\': out.append("\\\\"); break;
                case '\n': out.append("\\n"); break;
                case '\r': out.append("\\r"); break;
                case '\t': out.append("\\t"); break;
                default:
                    if (c < 0x20)
                        out.append(String.format("\\u%04x", (int)c));
                    else
                        out.append(c);
            }
        }
        return out.append("\"").toString();
    }
}
