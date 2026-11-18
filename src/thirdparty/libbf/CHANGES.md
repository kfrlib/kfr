# LibBF in KFR

Based on the 2025-06-03 LibBF release from Fabrice Bellard. Only the two library translation units and their headers are vendored; examples, benchmarks, and upstream tests are omitted. See [LICENSE.txt](LICENSE.txt).

- `cutils.h`: provide an MSVC implementation of compiler hints and bit scans. Use two 32-bit scans as needed so the 64-bit helpers also work on 32-bit targets.
- `cutils.h`: use `memcpy` for unaligned loads and stores instead of compiler-specific packed structs.
- `cutils.h`, `cutils.c`: retain the printf format annotation on compilers that support it.
- `libbf.h`: use upstream's 32-bit limb implementation for the MSVC ABI, including clang-cl, so no compiler-runtime 128-bit division helper is needed.
- `libbf.c`: use the AVX2 NTT implementation only with 64-bit limbs; use upstream's portable NTT otherwise.

The library is built only for KFR's opt-in math accuracy tests and is not installed with KFR.
