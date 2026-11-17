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
- Unit tests
  - JUnit test classes extend `BoofCPPStandardJUnit`, which has `rand`, `tolerance` and `assertMatches()`. Each
    module's `TestEnforceCodeStandards` checks it
  - gtest tests use `TEST_F` with a fixture derived from `BoofCPPTest`, which has `rng`, e.g.
    `class BlurImageOpsTest : public boofcv::BoofCPPTest {};`. `test_code_standards` checks it
  - Each variant's `TestHookContracts` calls every function of its hooks, and of the objects they create, with every
    gray image type. Each call must finish or throw a RuntimeException. Results aren't checked. Hooks that need
    specific argument values are listed in `customArguments` of `HookContractChecks`
- Benchmarks
  - JMH benchmark names are `<op>_<native|java>_<image type>`, e.g. `global_otsu_native_U8`, unless a `@Param`
    selects the image type
  - A short benchmark is written on one line
- JNI
  - C++ exceptions must never escape a JNI function. `jni_call()` turns them into Java exceptions
  - Hold critical arrays with `CriticalArray`, so an exception releases them
  - No JNI calls while a critical array (`GetPrimitiveArrayCritical`) is held
  - A JNI library contains all of its variant's code, so its native jar needs no other BoofCPP library.
    `./gradlew nativeJarCheck` loads each one from its jar alone
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
results/    Accuracy records, see Accuracy
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

- Use the same tests as strict, with a tolerance instead of exact comparisons
- Results must satisfy `|found - expected| <= tol*scale`, with `tol` 1e-5 for F32 and 1e-12 for F64
  - `scale` starts at 1. If a test fails, inspect the algorithm's logic before changing it
  - A `scale` other than 1 is justified in the test's documentation, preferably in 50 words or fewer
- Tests must include inputs which are hard for floating point: large values, cancellation, long sums, and image sizes
  which aren't a multiple of the vector width
- Tolerances are never widened, and shared tests aren't changed, without the maintainer's approval
- Shared tests
  - Shared test classes and headers end with `Checks`, so a search for `Test` finds each variant's own tests
  - C++: checks in `libs/strict/cpp/test/shared/` are written against namespace `tested` and compare with strict's
    naive implementations using the fast tolerance. A variant's test file includes its variant header, e.g.
    `variant_strict.h`, which sets `tested`, then the checks, and declares a test for each image type it supports
  - Java: abstract classes in `libs/common/jni/src/testFixtures` compare with BoofCV using `assertMatches()` from
    `BoofCPPStandardJUnit`. A variant's test class passes its hook and the image types it supports. Its `tolerance` is
    `Tolerance.IDENTICAL` by default, and fast's test classes set `Tolerance.FLOAT`
  - When fast adds an op, that op's strict tests are moved into shared tests first, without removing or weakening any
- Benchmark against strict's native code and BoofCV's Java code
- Fast's tests check its results. Fast's benchmarks call BoofCV after `BoofCPP.initialize()`, so they show if fast
  isn't hooked in
- When fast adds an op, the op is added to the accuracy tool of strict and fast
- `libs/fast/CLAUDE.md` has instructions for AI agents working on fast

# Accuracy

- Each variant's `accuracyRun` measures how far its results are from BoofCV's Java code, and saves a record in
  `results/accuracy/<variant>/<platform>.json`. Strict's metrics are all zero
- The shared cases are in `libs/common/jni/src/testFixtures`, e.g. `MeanBlurAccuracy`
- It only reports, including metrics which grew more than 2x since the saved record. It never fails a build
- Records are committed only for stable releases
- CI compiles it but doesn't run it
