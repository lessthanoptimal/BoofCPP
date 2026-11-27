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
  - Strict's multithreaded implementation must produce same output as single thread, even if BoofCV does not.
  - Fast reproduces BoofCV up to floating point tolerance and can produce different results depending on architecture.
    Namespace `boofcv::fast`. See [Fast](#fast)
- C++ standard is specified in the root `CMakeLists.txt`
- Java version will match BoofCV
- BoofCV's single-threaded Java code is the ground truth for results
  - Don't use multi-threaded BoofCV code for accuracy tests
- Multi-threaded / Concurrent Code
  - Has a code base separate from single threaded code
  - File names have the suffix `_mt` and class names `_MT`
  - Can and should share code with single threaded code, if it doesn't make that code substantially more complex
  - Unit tests compare its results with the single threaded version and run with multiple threads
  - JNI tests only check that hooks switch between it and single threaded code, with
    `BoofCPPTesting.assertConcurrentToggle()`, since the C++ tests cover its results
  - Number of threads is controlled centrally, similar to BoofCV's Java approach
  - Doesn't check the number of threads to call single threaded code instead of the MT logic; that belongs in JNI.
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
  - Provide pseudo code when an implementation is mathematically the same but deviates from the straightforward
    approach, e.g. working by block or 4 rows at a time. Show only the high-level flow of how the math is performed.
    When several implementations share a flow, state it once in the class documentation or refer to it. Don't restate
    intentional algorithm changes in BoofCV
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
  - A class that implements a BOverride interface has, in its class Javadoc, `/// BOOFCPP_OVERRIDE <BOverride
    class>.<field>` for each hook field it's registered with, or `/// BOOFCPP_IGNORE true` if it isn't registered.
    Each variant's `TestHookContracts` checks them against the hooks it registers
- Benchmarks
  - JMH benchmarks are in `libs/benchmark` and call BoofCV's API, so they time what users get. A benchmark method is
    one op, named after it, e.g. `global_otsu`, with a `@Hook` naming the hook field it uses. The implementation,
    image type and other settings are `@Param`s
  - `BenchmarkRunner` runs each op once for BoofCV's Java code and for each variant that has a hook for it. Only a
    run of every op is saved, in `results/benchmark/<platform>`, which `benchmarkReport` compares
  - A short benchmark is written on one line
- JNI
  - C++ exceptions must never escape a JNI function. `jni_call()` turns them into Java exceptions
  - Hold critical arrays with `CriticalArray`, so an exception releases them
  - No JNI calls while a critical array (`GetPrimitiveArrayCritical`) is held
  - Common's C++ code is a shared library that every variant's libraries link, so variants used together share one
    thread pool. A JNI library contains the rest of its variant's code, and its native jar also has the common
    library, so it needs no other jar. `./gradlew nativeJarCheck` loads each one from its jar alone
  - If a configuration isn't supported natively, BoofCV must fall back to its Java implementation, e.g. the override returns `null`

## Compositional testing

Test each piece of logic once, at the lowest layer that owns it

- C++
  - Tests use sub-images by default, so offsets and strides aren't the defaults
  - When possible, prove inner functions are correct with their own tests
  - Tests of functions that call them don't permute through inputs the inner function's tests already cover
  - Templated code is tested with integer and floating point types when their math differs
- Multithreaded code only demonstrates it produces the same results as the single threaded code, with multiple
  threads and blocks processed in different orders
- JNI
  - A few cases show the results match BoofCV's Java, identical for `strict` and within tolerance for `fast`
  - Multithreaded code only checks the switch between single threaded and multithreaded code
- Each unit test aims to run in under 0.2 s in a Release build. Mention tests that exceed this value after writing
  them.

# Directory Layout

```
libs/
  common/   Code independent of any variant, see Common below
  strict/   Results identical to BoofCV
  fast/     Results within floating point tolerance of BoofCV
  benchmark/ JMH benchmarks of BoofCV's Java code and every variant
android/    One module that packages the native libraries of every variant
benchmark/  C++ benchmarks
datasets/   Test images
results/    Accuracy and benchmark records, see Accuracy and Benchmarks
```

- Each library in `libs/`, except `benchmark/`, has `cpp/` and `jni/`
  - `cpp/` has `include/boofcv/` for headers, `src/` and `test/`
  - `jni/` is the Java module with its JNI code: `src/main/java`, `src/main/cpp` and `src/test`
  - JNI functions only tests use are in `src/test/cpp` and built into a test-only library, e.g. common's
    `JNIBoofCPPCommonTest`, never into a library that's shipped
- Fast uses strict's code for what it doesn't speed up, so fast depends on strict. Strict never depends on fast, and
  `common` never depends on a variant
- Variants have the same class and function names in their own namespace, so code and tests can switch between them
- Includes always start with `boofcv/`. Strict is `#include "boofcv/convolve.h"`, fast is
  `#include "boofcv/fast/convolve.h"`
- Each variant is its own native library and Java package, so several can be used in the same project.
  `BoofCPPStrict.initialize()` registers strict's hooks. Each other variant has its own `initialize()`, which replaces
  only the hooks it has, so the base hooks, e.g. strict's, are registered first
- New architecture specific variants follow the same layout

# Common (libs/common)

- Code that's independent of any implementation goes here: data structures, headers, glue such as JNI helpers, and
  test code, even if only one variant uses it so far
- Code specific to one variant, including its glue, doesn't go here
- Implementations of algorithms don't go here
  - Naive/brute force implementations used by unit tests are allowed, since they test the optimized implementations

# Fast

- Optimized for patterns which work on x64 and ARM64. Other architectures must build and be correct, but aren't tuned
- Anything goes as long as floating point results stay within tolerance of BoofCV
- Integer results are identical to strict
- No intrinsics. Code targeting a specific instruction set belongs in its own variant
- Must be at least 1.3x faster than strict's native code, measured with JMH. Otherwise use strict's
- Determinism
  - The same input, CPU and build always give the same output for single threaded code
  - Multithreaded code will ideally be deterministic, but can be non-deterministic if it's 1.3x faster than the
    deterministic version
    - Non-deterministic file names have the suffix `_mt_nd` and class names `_MT_ND`
  - If there is a non-deterministic multithreaded variant, there must be a deterministic one in `strict` or `fast` to
    compare its speed against
  - A central flag selects non-deterministic multithreaded code, if there is an implementation. It's off by default,
    so deterministic multithreaded code is used

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
  - C++: checks in `libs/common/cpp/test/shared/` are written against namespace `tested` and compare with the naive
    implementations in common, e.g. `convolve_naive.h`, using the fast tolerance. A variant's test file includes its
    variant header, e.g. `variant_strict.h`, which sets `tested`, then the checks, and declares a test for each image
    type it supports
  - Java: abstract classes in `libs/common/jni/src/testFixtures` compare with BoofCV using `assertMatches()` from
    `BoofCPPStandardJUnit`. A variant's test class passes its hook and the image types it supports. Its `tolerance` is
    `Tolerance.IDENTICAL` by default, and fast's test classes set `Tolerance.FLOAT`
  - When fast adds an op, that op's strict tests are moved into shared tests first, without removing or weakening any
- Benchmark against strict's native code and BoofCV's Java code
- Fast's tests check its results. The benchmarks in `libs/benchmark` run fast after strict, as a user registers them,
  and only on the ops it hooks
- When fast adds an op, the op is added to the accuracy tool of strict and fast
- Non-deterministic multithreaded code is run 10 times with more threads than cores. Each result must be within
  floating point tolerance of the single threaded implementation
- `libs/fast/CLAUDE.md` has instructions for AI agents working on fast

# Accuracy

- Each variant's `accuracyRun` measures how far its results are from BoofCV's Java code, and saves a record in
  `results/accuracy/<variant>/<platform>.json`. Strict's metrics are all zero
- The shared cases are in `libs/common/jni/src/testFixtures`, e.g. `MeanBlurAccuracy`
- It only reports, including metrics which grew more than 2x since the saved record. It never fails a build
- Records are committed only for stable releases
- CI compiles it but doesn't run it
