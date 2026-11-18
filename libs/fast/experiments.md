# Experiments

Ideas tried in fast that weren't kept, and why. Read it before trying an op again.

## ConvolveImage horizontal and vertical, F32 (2026-11-17)

Not kept: no technique allowed on x86-64-v2 makes fast 1.3x faster than strict. Strict already vectorizes the inner
loops: kernel widths 3 to 13 are unrolled at compile time, and vertical sums columns in blocks of 256.

C++, 4032x3024 test image, Gaussian kernels, median of 15, one core of a Ryzen 9 PRO 8945HS (ms):

| radius | H strict | H, each weight over a block of pixels | H, symmetric pairs added first | V strict | V, 4 output rows per pass |
|--------|----------|---------------------------------------|--------------------------------|----------|---------------------------|
| 1      | 3.6      | 5.5                                   | 5.0                            | 12.7     | 10.5                      |
| 2      | 4.0      | 7.2                                   | 6.6                            | 14.5     | 13.0                      |
| 5      | 7.8      | 13.6                                  | 11.1                           | 20.0     | 20.2                      |
| 10     | 74.3     | 22.7                                  | 16.7                           | 31.5     | 33.7                      |

- Processing each weight over a block of pixels gives results identical to strict, so it went into strict for kernels
  wider than 13, which strict doesn't unroll (2.4x to 3.0x faster there).
- Adding symmetric pairs before the multiply changes the rounding. It only helps wide kernels.
- Four output rows per pass gives identical results and helps only radius 1 and 2 (1.1x to 1.2x).
- AVX2 (`-march=x86-64-v3`) makes strict's own code 1.3x to 1.75x faster at radius 5 and 10, with identical results.
  FMA adds little. BoofCPP's default instruction set stays x86-64-v2, so this is a build option, see `BOOFCPP_MARCH`
  in the readme.

## ConvolveImage 2D, F32 (2026-11-17)

Not kept: strict's code for centered kernels 3 to 11 wide sums each kernel row with a width known at compile time, the
same as BoofCV, and nothing that changes the rounding was faster. C++, 4032x3024, Gaussian kernels (ms):

| width | strict | one sum per pixel, a block of pixels at once | strict's row order, rows added in a block buffer |
|-------|--------|----------------------------------------------|--------------------------------------------------|
| 3     | 10.0   | 16.7                                         | 12.8                                             |
| 7     | 32.8   | 57.2                                         | 39.6                                             |
| 11    | 76.0   | 128.8                                        | 114.2                                            |

- For wider or off-center kernels, a block of pixels at once gives results identical to strict, so it went into strict
  (4.4x to 5.5x faster for F32 at widths 13 and 21).
- Not tried: a 2D kernel that is the outer product of two 1D kernels, e.g. a Gaussian, could run as a horizontal pass
  and then a vertical pass, about 2.7x faster at width 11. Callers with such kernels can already call the 1D functions.

## BlurImageOps.gaussian, F32 (2026-11-17)

Not kept: below 1.3x at radius 5, the radius of the JMH benchmark. The Gaussian is strict's horizontal and vertical
normalized convolution, see ConvolveImage above. C++, 4032x3024, inner region (ms):

| radius | strict H + V | fused in strips of 128 rows | V strict | V, mirrored rows added first | estimated total with that V |
|--------|--------------|-----------------------------|----------|------------------------------|-----------------------------|
| 2      | 18.4         | 15.7                        | 14.5     | 11.6                         | 15.6 (1.19x)                |
| 5      | 27.9         | 25.8                        | 20.2     | 15.2                         | 23.0 (1.22x)                |
| 10     | 57.6         | 58.9                        | 31.6     | 23.0                         | 39.6 (1.37x, H also folded) |

- Fusing the passes, so the horizontal rows are still in cache for the vertical pass, gives identical results but
  recomputes width-1 rows per strip. It helps at most 1.18x.
- Adding mirrored rows of the symmetric kernel before the multiply changes the rounding (at most 7.6e-5 on 0..255) and
  only reaches 1.3x for large radii. Folding the horizontal pass is slower than strict at radius 5.

## Census transform dense3x3, F32 (2026-11-18)

Not kept: comparing 8 neighbors per pass into the byte output, which makes U8 4.6x and U16 2.2x faster than strict, is
only 1.18x faster for F32 (13.1 vs 15.6 ms, JMH, 4032x3024 test image, reflected border). Each F32 comparison gives a
32-bit mask that is narrowed to a byte, and strict's output is already a byte written once per neighbor. Fast's
dense3x3 uses strict's code for F32.

## Stereo block matching (2026-11-18)

Not kept, from C++ prototypes on the benchmark's stereo pair (672x504, range 100, 7x7 region, every check on, ms):

| Approach | SAD U8 | census 5x5 | SAD F32 | Why not |
|----------|--------|------------|---------|---------|
| strict | 77 | 72 | 83 | |
| kept: fast's selector, 16-bit integer scores | 37 | 46 | 58 | |
| row scores as a box sum of shifted errors | 36 | 45 | 65 | F32 score error 1.45e-5 x max(1,\|s\|), outside tolerance. Integers within 3% of the running sum |
| vertical running sum fused into the row scores | 36 | 45 | | no change |
| every disparity of a pixel in contiguous 16-bit lanes, vertical sums first, a ring of per pixel errors | 32 | 45 | | only SAD gains, about 10%, and only for 16-bit scores, with more new code |
| the same with SSE4.1 intrinsics, x86-64-v3 build | 23 | | | no intrinsics in fast. The portable version is also 23 ms built for x86-64-v3 |

- The selector's loops go over every column for one disparity, contiguous in the score layout. GCC vectorizes the
  argmin of float scores only when the index is blended with an explicit mask, not with `?:`.
- 16-bit scores are exact when the region's largest score and the range are below 65535, since the running sums are
  then exact modulo 2^16. Otherwise int32_t, where the selector alone is 1.43x for SAD U8.

## Thresholds (2026-11-18)

Not kept, C++ on the 4032x3024 test image:

- Global fixed threshold: not attempted. Strict's loop vectorizes and runs at memory speed (0.68 ms U8, 2.8 ms F32).
- Global Otsu, F32: four histogram copies make the histogram 1.5x faster (10.9 to 7.3 ms), but thresholding the image
  (2.7 ms) stays, about 1.15x in total. Fast uses strict's code for F32.
