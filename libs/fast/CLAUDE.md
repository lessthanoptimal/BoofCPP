# Instructions for AI agents working on fast

Fast is developed with less human supervision than strict. The tests enforce that it's correct, so they are never
weakened to make code pass. Read `CodeStandards.md` in the repository root first, especially
[Directory Layout](../../CodeStandards.md#directory-layout) and [Fast](../../CodeStandards.md#fast).

## Workflow for each function

1. Start from the strict function, its tests and its benchmarks. The fast version has the same name and signature in
   namespace `boofcv::fast`, under `libs/fast/cpp/include/boofcv/fast/`
2. Implement it. Anything goes within floating point tolerance. Intrinsics need a portable C++ fallback, and both are
   tested
3. Test it with strict's tests, using the fast tolerance. Add inputs which are hard for floating point if the tests
   don't have them yet
4. Self-review against `CodeStandards.md` and the design goals: within tolerance, integers identical to strict,
   deterministic, simple and readable, no duplicated code, shared code in `libs/common`
5. Fix every issue the review found, then review again until nothing is left
6. Run every C++ and JNI test. All of them must pass
7. Benchmark against strict's native code and BoofCV's Java code. Keep the function only if it's at least 1.5x faster
   than strict. Otherwise remove it and record why. If it combines techniques, benchmark each one alone, see
   [Prefer simple code](#prefer-simple-code)
8. Commit with the speedup over strict and Java, and the kept techniques with their share of it, in the message.
   Intermediate commits are fine for saving progress. Once the approach is final, squash them into one commit per op,
   e.g. one for the Gaussian blur

## Prefer simple code

Keep complexity only when it measurably adds speed. When an op combines techniques (e.g. intrinsics and a new memory
access order), benchmark each one alone. Drop any whose contribution is within noise.

## Stop and ask the maintainer

- A shared test or a tolerance would need to change
- A result is outside tolerance and the cause isn't found
- `libs/common` or `libs/strict` would need to change
