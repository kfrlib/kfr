# Known bugs and limitations

| Compiler | Architecture | ISA | Description
| - | - | - | - |
| Visual Studio | x86/x86_64 | < SSE2 | Not supported, SSE2 is required |
| GCC | x86/x86_64 | < SSE2 | Not supported, SSE2 is required |
| Visual Studio 2022 | x86_64 | AVX512 | Internal Compiler Error (sometimes) |
| Clang 14 | x86/x86_64 | Generic | Code generation bug in Clang |
| GCC 12 | x86/x86_64 | AVX512 | Code generation bug in GCC |
| Clang 10 | ARM, ARM64 | | Code generation bug in Clang https://github.com/kfrlib/kfr/issues/112 |
| Clang 13+ | ARM, ARM64 | | Code generation bug in Clang in `kfr::exp` |
