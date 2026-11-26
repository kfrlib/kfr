# Known bugs and limitations

| Compiler           | Architecture | ISA     | Description                                                           |
|--------------------|--------------|---------|-----------------------------------------------------------------------|
| Visual Studio      | x86/x86_64   | < SSE2  | Not supported, SSE2 is required                                       |
| GCC                | x86/x86_64   | < SSE2  | Not supported, SSE2 is required                                       |
| Visual Studio 2022 | x86_64       | AVX512  | Internal Compiler Error (sometimes)                                   |
| MSVC 195237029     | Various      | Various | Several internal compiler errors and miscompilation bugs; see below    |
| Clang 14           | x86/x86_64   | Generic | Code generation bug in Clang                                          |
| GCC 12             | x86/x86_64   | AVX512  | Code generation bug in GCC                                            |
| Clang 10           | ARM, ARM64   |         | Code generation bug in Clang https://github.com/kfrlib/kfr/issues/112 |
| Clang 13+          | ARM, ARM64   |         | Code generation bug in Clang in `kfr::exp`                            |

MSVC 195237029 can generate AVX2 instructions in AVX mode. Intel SDE reports:

```text
TID 0 SDE-ERROR: Executed instruction not valid for specified chip (SANDYBRIDGE): 0x7ff60c61c626: vpxor ymm0, ymm0, ymm0
```
