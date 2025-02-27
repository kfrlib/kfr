# Benchmarking DFT

Accurate DFT benchmarking requires careful control of optimizations, CPU architecture, and compiler behavior. Follow these guidelines to ensure reliable performance measurements.

> [!note]
> A robust FFT benchmark suite implementing all these techniques is published at https://github.com/kfrlib/fft-benchmark

- Ensure that the optimized version of each library is used. If the vendor provides prebuilt binaries, use them.
  - For KFR, the official binaries can be found at: https://github.com/kfrlib/kfr/releases.
  - To verify that KFR is optimized for maximum performance, call:

  ```c++
  library_version()
  ```
  Example output:
  ```
  KFR 6.1.1 optimized sse2 [sse2, sse41, avx, avx2, avx512] 64-bit (clang-msvc-19.1.0/windows) +in +ve
  ```
  The output must include the `optimized` flag and must not contain the `debug` flag.

- For libraries that support dynamic CPU dispatch, ensure that the best available architecture for your CPU is selected at runtime. Refer to the library documentation to learn how to verify this.
  - For KFR, call:

  ```c++
  cpu_runtime()
  ```
  This function returns the selected architecture, such as `avx2`, `avx512`, or `neon`/`neon64` (for ARM).

- Ensure that no emulation is involved. For example, use native `arm64` binaries for Apple M-series CPUs.

- Exclude plan creation from the time measurements.

- Ensure that the compiler does not optimize out the DFT code. Add code that utilizes the output data in some way to prevent the compiler from optimizing away the computation.

- Perform multiple invocations to obtain reliable results. A few seconds of execution time is the minimum requirement for accurate measurements.

- Use the median or minimum of all measured execution times rather than the simple mean, as this better protects against unexpected spikes and benchmarking noise.

