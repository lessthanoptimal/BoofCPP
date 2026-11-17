# General

- Only update 3rd party dependencies which are not BoofCV infrequently
  - Typically every year or two
- Do not update CMake unless necessary
- Gradle wrapper uses the same version as BoofCV

# BoofCV

- Concurrency: Single and multithread code are both deterministic when given the same input/configuration and produce the same output
  - If this is discovered to not be the case it is most likely a bug, review BoofCV's code to see if it's intentional
  - Rule can be broken but needs an extremely good reason and must be documented

# BoofCPP

- Pure C++ implementation is designed as a standalone library that can be used by itself without Java BoofCV
  - Secondary goal and for now isn't fully fleshed out
- Two C++ libraries are provided, `strict` and `fast`. See [Directory Layout](#directory-layout)
  - Strict reproduces exact same results of BoofCV, warts and all. Namespace `boofcv`
  - Fast reproduces BoofCV up to floating point tolerance and can produce different results depending on architecture.
    Namespace `boofcv::fast`. See [Fast](#fast)
- C++ standard is specified in the root `CMakeLists.txt`
- Java version will match BoofCV
- Library version will match BoofCV's version
- Templates are only used when they greatly simplify the code, e.g. image data types
- Use virtual functions for polymorphism is preferred over templates except in hot loops or when it makes it more complex
- Keep C++ code simple and prefer safe code unless it's proven to be a performance hit
  - Don't blindly follow C++ performance rules of thumb, prove them with benchmarks
  - Humans: That expert who said you should always use complex C++ idioms because they are faster is prematurely optimizing, writes unmaintainable code to feel smart, and should be ignored
- Comments
  - C++ uses Doxygen style comments with ///
  - Java uses Markdown Javadoc
  - Comments describe the code as it is, not its history
- Configurations
  - C++ configurations have the same field names (in snake case) and default values as the Java configuration
  - They implement `visit_fields()`, which lists every field with its Java name
  - A test checks the field names and defaults against Java and that every field survives a copy to C++ and back
- JNI
  - C++ exceptions must never escape a JNI function. Catch them and throw a Java exception instead
  - No JNI calls while a critical array (`GetPrimitiveArrayCritical`) is held
  - If a configuration isn't supported natively, BoofCV must fall back to its Java implementation, e.g. the override returns `null`

# Directory Layout

```
libs/
  common/   Code every variant shares: image types, borders, configurations, JNI helpers, test helpers
  strict/   Results identical to BoofCV
  fast/     Results within floating point tolerance of BoofCV
android/    One module that packages the native libraries of every variant
benchmark/  C++ benchmarks
datasets/   Test images
```

- Each library in `libs/` has `cpp/` and `jni/`
  - `cpp/` has `include/boofcv/` for headers, `src/` and `test/`
  - `jni/` is the Java module with its JNI code: `src/main/java`, `src/main/cpp`, `src/test` and `src/benchmark`
- `common` never depends on a variant, and the variants don't depend on each other
- Variants have the same class and function names in their own namespace, so code and tests can switch between them
- Includes always start with `boofcv/`. Strict is `#include "boofcv/convolve.h"`, fast is
  `#include "boofcv/fast/convolve.h"`
- Each variant is its own native library and Java package, so several can be used in the same project.
  `BoofCPP.initialize()` registers the strict hooks and then the hooks of every other variant that's available, each
  replacing only the hooks it has
- New architecture specific variants follow the same layout

# Fast

- Optimized for patterns which work on x64 and ARM64. Other architectures must build and be correct, but aren't tuned
- Anything goes as long as floating point results stay within tolerance of BoofCV
- Integer results are identical to strict
- Intrinsics are allowed, but only with a portable C++ fallback
- Must be at least 1.3x faster than strict's native code, measured with JMH. Otherwise use strict's
- Deterministic: the same input, CPU and build always give the same output, single or multi-threaded

## Testing Fast

- Use the same tests as strict, with a tolerance instead of exact comparisons:
  `|found - expected| <= tol*max(1, |expected|)` with `tol` 1e-5 for F32 and 1e-12 for F64
- Tests must include inputs which are hard for floating point: large values, cancellation, long sums, and image sizes
  which aren't a multiple of the vector width
- Tolerances are never widened, and shared tests aren't changed, without the maintainer's approval
- Benchmark against strict's native code and BoofCV's Java code
- `libs/fast/CLAUDE.md` has instructions for AI agents working on fast
