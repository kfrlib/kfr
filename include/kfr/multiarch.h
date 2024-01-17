#include "cident.h"

#ifdef CMT_ARCH_X86

// x86

#ifdef CMT_MULTI_ENABLED_AVX512
#define CMT_IF_ENABLED_AVX512(...) __VA_ARGS__
#else
#define CMT_IF_ENABLED_AVX512(...)
#endif

#ifdef CMT_MULTI_ENABLED_AVX2
#define CMT_IF_ENABLED_AVX2(...) __VA_ARGS__
#else
#define CMT_IF_ENABLED_AVX2(...)
#endif

#ifdef CMT_MULTI_ENABLED_AVX
#define CMT_IF_ENABLED_AVX(...) __VA_ARGS__
#else
#define CMT_IF_ENABLED_AVX(...)
#endif

#ifdef CMT_MULTI_ENABLED_SSE42
#define CMT_IF_ENABLED_SSE42(...) __VA_ARGS__
#else
#define CMT_IF_ENABLED_SSE42(...)
#endif

#ifdef CMT_MULTI_ENABLED_SSE41
#define CMT_IF_ENABLED_SSE41(...) __VA_ARGS__
#else
#define CMT_IF_ENABLED_SSE41(...)
#endif

#ifdef CMT_MULTI_ENABLED_SSSE3
#define CMT_IF_ENABLED_SSSE3(...) __VA_ARGS__
#else
#define CMT_IF_ENABLED_SSSE3(...)
#endif

#ifdef CMT_MULTI_ENABLED_SSE3
#define CMT_IF_ENABLED_SSE3(...) __VA_ARGS__
#else
#define CMT_IF_ENABLED_SSE3(...)
#endif

#ifdef CMT_MULTI_ENABLED_SSE2
#define CMT_IF_ENABLED_SSE2(...) __VA_ARGS__
#else
#define CMT_IF_ENABLED_SSE2(...)
#endif

#ifdef CMT_MULTI

#define CMT_MULTI_PROTO_GATE(...)                                                                            \
    if (cpu == cpu_t::runtime)                                                                               \
        cpu = get_cpu();                                                                                     \
    switch (cpu)                                                                                             \
    {                                                                                                        \
    case cpu_t::avx512:                                                                                      \
        CMT_IF_ENABLED_AVX512(return avx512::__VA_ARGS__;)                                                   \
    case cpu_t::avx2:                                                                                        \
        CMT_IF_ENABLED_AVX2(return avx2::__VA_ARGS__;)                                                       \
    case cpu_t::avx:                                                                                         \
        CMT_IF_ENABLED_AVX(return avx::__VA_ARGS__;)                                                         \
    case cpu_t::sse42:                                                                                       \
        CMT_IF_ENABLED_SSE42(return sse42::__VA_ARGS__;)                                                     \
    case cpu_t::sse41:                                                                                       \
        CMT_IF_ENABLED_SSE41(return sse41::__VA_ARGS__;)                                                     \
    case cpu_t::ssse3:                                                                                       \
        CMT_IF_ENABLED_SSSE3(return ssse3::__VA_ARGS__;)                                                     \
    case cpu_t::sse3:                                                                                        \
        CMT_IF_ENABLED_SSE3(return sse3::__VA_ARGS__;)                                                       \
    case cpu_t::sse2:                                                                                        \
        CMT_IF_ENABLED_SSE2(return sse2::__VA_ARGS__;)                                                       \
    default:                                                                                                 \
        CMT_UNREACHABLE;                                                                                     \
    }

#define CMT_MULTI_GATE(...)                                                                                  \
    switch (get_cpu())                                                                                       \
    {                                                                                                        \
    case cpu_t::avx512:                                                                                      \
        CMT_IF_ENABLED_AVX512({                                                                              \
            namespace ns = kfr::avx512;                                                                      \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::avx2:                                                                                        \
        CMT_IF_ENABLED_AVX2({                                                                                \
            namespace ns = kfr::avx2;                                                                        \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::avx:                                                                                         \
        CMT_IF_ENABLED_AVX({                                                                                 \
            namespace ns = kfr::avx;                                                                         \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::sse42:                                                                                       \
        CMT_IF_ENABLED_SSE42({                                                                               \
            namespace ns = kfr::sse42;                                                                       \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::sse41:                                                                                       \
        CMT_IF_ENABLED_SSE41({                                                                               \
            namespace ns = kfr::sse41;                                                                       \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::ssse3:                                                                                       \
        CMT_IF_ENABLED_SSSE3({                                                                               \
            namespace ns = kfr::ssse3;                                                                       \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::sse3:                                                                                        \
        CMT_IF_ENABLED_SSE3({                                                                                \
            namespace ns = kfr::sse3;                                                                        \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::sse2:                                                                                        \
        CMT_IF_ENABLED_SSE2({                                                                                \
            namespace ns = kfr::sse2;                                                                        \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    default:                                                                                                 \
        CMT_UNREACHABLE;                                                                                     \
    }

#define CMT_MULTI_PROTO(...)                                                                                 \
    CMT_IF_ENABLED_SSE2(CMT_IF_IS_SSE2(inline) namespace sse2{ __VA_ARGS__ })                                \
    CMT_IF_ENABLED_SSE3(CMT_IF_IS_SSE3(inline) namespace sse3{ __VA_ARGS__ })                                \
    CMT_IF_ENABLED_SSSE3(CMT_IF_IS_SSSE3(inline) namespace ssse3{ __VA_ARGS__ })                             \
    CMT_IF_ENABLED_SSE42(CMT_IF_IS_SSE42(inline) namespace sse42{ __VA_ARGS__ })                             \
    CMT_IF_ENABLED_SSE41(CMT_IF_IS_SSE41(inline) namespace sse41{ __VA_ARGS__ })                             \
    CMT_IF_ENABLED_AVX(CMT_IF_IS_AVX(inline) namespace avx{ __VA_ARGS__ })                                   \
    CMT_IF_ENABLED_AVX2(CMT_IF_IS_AVX2(inline) namespace avx2{ __VA_ARGS__ })                                \
    CMT_IF_ENABLED_AVX512(CMT_IF_IS_AVX512(inline) namespace avx512{ __VA_ARGS__ })
#else
#define CMT_MULTI_GATE(...)                                                                                  \
    do                                                                                                       \
    {                                                                                                        \
        namespace ns = kfr::CMT_ARCH_NAME;                                                                   \
        __VA_ARGS__;                                                                                         \
        break;                                                                                               \
    } while (0)

#define CMT_MULTI_PROTO(...)                                                                                 \
    inline namespace CMT_ARCH_NAME                                                                           \
    {                                                                                                        \
    __VA_ARGS__                                                                                              \
    }
#endif

#if defined(CMT_BASE_ARCH) || !defined(CMT_MULTI)
#define CMT_MULTI_NEEDS_GATE
#else
#endif

#else

// ARM

#define CMT_MULTI_PROTO_GATE(...)                                                                            \
    do                                                                                                       \
    {                                                                                                        \
        return CMT_ARCH_NAME::__VA_ARGS__;                                                                   \
    } while (0)

#define CMT_MULTI_GATE(...)                                                                                  \
    do                                                                                                       \
    {                                                                                                        \
        namespace ns = kfr::CMT_ARCH_NAME;                                                                   \
        __VA_ARGS__;                                                                                         \
        break;                                                                                               \
    } while (0)

#define CMT_MULTI_PROTO(...)                                                                                 \
    inline namespace CMT_ARCH_NAME                                                                           \
    {                                                                                                        \
    __VA_ARGS__                                                                                              \
    }

#if defined(CMT_BASE_ARCH) || !defined(CMT_MULTI)
#define CMT_MULTI_NEEDS_GATE
#else
#endif

#endif
