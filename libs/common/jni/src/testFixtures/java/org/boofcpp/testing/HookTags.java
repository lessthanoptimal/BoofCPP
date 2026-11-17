package org.boofcpp.testing;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Stream;

/// The hook tags of a top-level class, read from its source file. A class that implements a BOverride interface has,
/// in its class Javadoc, `/// BOOFCPP_OVERRIDE <BOverride class>.<field>` for each hook field it's registered with, or
/// `/// BOOFCPP_IGNORE true` if it isn't registered
public class HookTags {
    private static final Pattern PACKAGE = Pattern.compile("^package\\s+([\\w.]+)\\s*;", Pattern.MULTILINE);
    private static final Pattern CLASS = Pattern.compile(
            "^(?:(?:public|abstract|final)\\s+)*(?:class|interface|enum)\\s+(\\w+)", Pattern.MULTILINE);
    private static final Pattern INTERFACE = Pattern.compile("\\b(BOverride\\w+\\.\\w+)");
    private static final Pattern TAG = Pattern.compile("^///\\s*(BOOFCPP_\\w+)\\s*(.*?)\\s*$");

    /// Fully qualified name of the class
    public final String className;
    /// Hook fields it's registered with, e.g. "BOverrideBlurImageOps.mean"
    public final List<String> overrides = new ArrayList<>();
    /// True if it isn't registered with any hook field
    public boolean ignore;
    /// BOverride interfaces it implements, e.g. "BOverrideBlurImageOps.Mean"
    public final List<String> interfaces = new ArrayList<>();

    HookTags( String className ) {
        this.className = className;
    }

    /// The tags of the source's top-level class, or null if it has none. Throws an IllegalArgumentException for an
    /// unknown BOOFCPP_ tag, e.g. a misspelled one
    public static HookTags parse( String source ) {
        Matcher declaration = CLASS.matcher(source);
        if (!declaration.find())
            return null;
        Matcher pkg = PACKAGE.matcher(source);
        String name = (pkg.find() ? pkg.group(1) + "." : "") + declaration.group(1);
        var tags = new HookTags(name);

        int bodyStart = source.indexOf('{', declaration.start());
        Matcher implemented = INTERFACE.matcher(source.substring(declaration.start(), bodyStart));
        while (implemented.find()) {
            tags.interfaces.add(implemented.group(1));
        }

        // The class Javadoc is the /// lines right before the declaration and its annotations
        String[] before = source.substring(0, declaration.start()).split("\n");
        for (int i = before.length - 1; i >= 0; i--) {
            String line = before[i].trim();
            if (line.startsWith("@"))
                continue;
            if (!line.startsWith("///"))
                break;
            Matcher tag = TAG.matcher(line);
            if (!tag.matches())
                continue;
            if (tag.group(1).equals("BOOFCPP_OVERRIDE"))
                tags.overrides.add(0, tag.group(2));
            else if (tag.group(1).equals("BOOFCPP_IGNORE") && tag.group(2).equals("true"))
                tags.ignore = true;
            else
                throw new IllegalArgumentException("Unknown tag in " + name + ": " + line);
        }
        return tags;
    }

    /// The tags of every class in the source directory
    public static List<HookTags> load( File directory ) throws IOException {
        List<HookTags> all = new ArrayList<>();
        try (Stream<Path> files = Files.walk(directory.toPath())) {
            for (Path path : (Iterable<Path>)files.filter(p -> p.toString().endsWith(".java"))::iterator) {
                HookTags tags = parse(new String(Files.readAllBytes(path), "UTF-8"));
                if (tags != null)
                    all.add(tags);
            }
        }
        return all;
    }
}
