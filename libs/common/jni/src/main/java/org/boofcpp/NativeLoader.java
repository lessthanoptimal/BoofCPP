package org.boofcpp;

import cz.adamh.utils.NativeUtils;

import java.io.File;

/// Loads a variant's JNI library from the library path, the native build directory when run inside the repository, or
/// the native jar. The JNI libraries link the common library, which has the thread pool that every variant shares, so
/// it's loaded first from the same places. The JNI library then uses the loaded one wherever it came from
public class NativeLoader {
    private static boolean commonLoaded = false;

    /// @param name Base name of the library, e.g. JNIBoofCPP
    /// @param directory Its directory inside the native build directory, e.g. boofcpp-jni
    /// @return true if it loaded
    public static synchronized boolean load( String name, String directory ) {
        if (!commonLoaded)
            commonLoaded = loadLibrary("BoofCPPCommon", "boofcpp-common");
        return loadLibrary(name, directory);
    }

    private static boolean loadLibrary( String name, String directory ) {
        try {
            System.loadLibrary(name);
            return true;
        } catch (UnsatisfiedLinkError ignore) {}

        try {
            NativeUtils.setLibraryName(name);
            for (String root : new String[]{"build", "../build", "../../../build"}) {
                if (NativeUtils.loadLocalPath(new File(root, directory)))
                    return true;
            }
            for (String path : new String[]{"/", "/arm64-v8a", "/armeabi-v7a"}) {
                if (NativeUtils.loadLibraryFromJar(path))
                    return true;
            }
        } catch (Exception | UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
        System.err.println("Failed to load native library " + name);
        return false;
    }

    /// Loads the library given by the arguments, e.g. `JNIBoofCPP boofcpp-jni`, and exits with 1 if it fails. The
    /// nativeJarCheck task runs it with only the native jar, to check that the jar works on its own
    public static void main( String[] args ) {
        if (!load(args[0], args[1]))
            System.exit(1);
        System.out.println("Loaded " + args[0]);
    }
}
