package org.boofcpp.benchmark;

import org.openjdk.jmh.results.RunResult;
import org.openjdk.jmh.results.format.ResultFormatFactory;
import org.openjdk.jmh.results.format.ResultFormatType;
import org.openjdk.jmh.runner.BenchmarkList;
import org.openjdk.jmh.runner.BenchmarkListEntry;
import org.openjdk.jmh.runner.Runner;
import org.openjdk.jmh.runner.format.OutputFormat;
import org.openjdk.jmh.runner.format.OutputFormatFactory;
import org.openjdk.jmh.runner.options.OptionsBuilder;
import org.openjdk.jmh.runner.options.TimeValue;
import org.openjdk.jmh.runner.options.VerboseMode;

import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

/// Runs each benchmarked op once for every implementation that has it: BoofCV's Java code always, and a variant only
/// if registering it changes the op's [Hook] field from that of its base implementation. Each implementation's results
/// are written to `<impl>.json` in JMH's format, with `<impl>.metadata.json`, see [RunMetadata].
///
/// Options:
/// - `--out <dir>` Where the results are written
/// - `--include <regex>` Only the benchmarks that match. Without it, every op runs
/// - `--impl <names>` Only these implementations, separated by commas. Without it, every supported one runs
/// - `--threads <counts>` Thread counts, separated by commas. Without it, 1 and every core the JVM sees
/// - `--quick true` One short measurement without warmup, to check that the benchmarks run. Can't be saved
/// - `--save <dir>` Also copies the files of each implementation that ran into this directory, replacing the saved
///   ones. Only allowed without `--include`, so a saved file always has every op
public class BenchmarkRunner {
    public static void main( String[] args ) throws Exception {
        Map<String, String> options = parse(args);
        File out = new File(options.getOrDefault("out", "."));
        String include = options.get("include");
        File save = options.containsKey("save") ? new File(options.get("save")) : null;
        boolean quick = Boolean.parseBoolean(options.getOrDefault("quick", "false"));
        if (save != null && include != null)
            throw new IllegalArgumentException("Only a run of every op can be saved, so --save can't have --include");
        if (save != null && quick)
            throw new IllegalArgumentException("A quick run can't be saved");
        List<Implementation> requested = requested(options.get("impl"));
        String[] threads = threads(options.get("threads"));

        OutputFormat format = OutputFormatFactory.createFormatInstance(System.out, VerboseMode.NORMAL);
        List<RunResult> results = new ArrayList<>();
        for (BenchmarkListEntry entry : BenchmarkList.defaultList()
                .find(format, List.of(include == null ? ".*" : include), List.of())) {
            String name = entry.getUsername();
            List<String> implementations = new ArrayList<>();
            for (Implementation i : implementationsOf(name)) {
                if (requested.contains(i))
                    implementations.add(i.name());
            }
            System.out.println("# " + name + ": " + implementations);
            if (implementations.isEmpty())
                continue;
            var jmh = new OptionsBuilder()
                    .include("^" + Pattern.quote(name) + "$")
                    .param("impl", implementations.toArray(new String[0]))
                    .param("threads", threads);
            if (quick)
                jmh.warmupIterations(0).measurementIterations(1).measurementTime(TimeValue.milliseconds(100));
            results.addAll(new Runner(jmh.build()).run());
        }

        out.mkdirs();
        if (save != null)
            save.mkdirs();
        for (Implementation implementation : requested) {
            List<RunResult> own = new ArrayList<>();
            for (RunResult r : results) {
                if (r.getParams().getParam("impl").equals(implementation.name()))
                    own.add(r);
            }
            if (own.isEmpty())
                continue;
            File resultFile = new File(out, implementation + ".json");
            File metadataFile = new File(out, implementation + ".metadata.json");
            try (var stream = new PrintStream(resultFile, StandardCharsets.UTF_8)) {
                ResultFormatFactory.getInstance(ResultFormatType.JSON, stream).writeOut(own);
            }
            Files.writeString(metadataFile.toPath(), RunMetadata.toJson(RunMetadata.of(implementation)) + "\n");
            System.out.println("Wrote " + resultFile.getAbsolutePath());
            if (save != null) {
                copy(resultFile, save);
                copy(metadataFile, save);
                System.out.println("Saved " + implementation + " in " + save.getAbsolutePath());
            }
        }
    }

    private static void copy( File file, File directory ) throws IOException {
        Files.copy(file.toPath(), new File(directory, file.getName()).toPath(), StandardCopyOption.REPLACE_EXISTING);
    }

    /// `--name value` pairs
    static Map<String, String> parse( String[] args ) {
        Map<String, String> options = new LinkedHashMap<>();
        for (int i = 0; i < args.length; i += 2) {
            if (!args[i].startsWith("--") || i + 1 == args.length)
                throw new IllegalArgumentException("Expected --name value, found " + args[i]);
            options.put(args[i].substring(2), args[i + 1]);
        }
        return options;
    }

    /// The thread counts in 'counts', separated by commas, or 1 and every core the JVM sees if it's null
    static String[] threads( String counts ) {
        if (counts != null)
            return counts.split(",");
        int cores = Runtime.getRuntime().availableProcessors();
        return cores == 1 ? new String[]{"1"} : new String[]{"1", String.valueOf(cores)};
    }

    /// The implementations named in 'names', separated by commas, or every supported one if it's null. Throws if one
    /// is unknown or doesn't run here
    static List<Implementation> requested( String names ) {
        List<Implementation> found = new ArrayList<>();
        if (names == null) {
            for (Implementation i : Implementation.values()) {
                if (i.isSupported())
                    found.add(i);
            }
            return found;
        }
        for (String name : names.split(",")) {
            Implementation i = Implementation.valueOf(name.trim());
            if (!i.isSupported())
                throw new IllegalArgumentException(name + " doesn't run on this machine");
            found.add(i);
        }
        return found;
    }

    /// Implementations that have the op, e.g. "org.boofcpp.benchmark.threshold.ThresholdingBenchmark.global_otsu"
    static List<Implementation> implementationsOf( String benchmark ) throws Exception {
        int split = benchmark.lastIndexOf('.');
        Method method = Class.forName(benchmark.substring(0, split)).getMethod(benchmark.substring(split + 1));
        Hook hook = method.getAnnotation(Hook.class);
        if (hook == null)
            throw new IllegalArgumentException(benchmark + " has no @Hook");

        Field field = hook.type().getField(hook.field());
        List<Implementation> found = new ArrayList<>();
        for (Implementation implementation : Implementation.values()) {
            if (!implementation.isSupported())
                continue;
            if (implementation.base == null) {
                found.add(implementation);
                continue;
            }
            implementation.base.register();
            Object before = field.get(null);
            implementation.registerOwn();
            if (field.get(null) != before)
                found.add(implementation);
        }
        Implementation.java.register();
        return found;
    }
}
