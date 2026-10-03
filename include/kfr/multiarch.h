#include "cident.h"

#ifdef KFR_ARCH_X86

// x86

#ifdef KFR_MULTI_ENABLED_AVX512
#define KFR_IF_ENABLED_AVX512(...) __VA_ARGS__
#else
#define KFR_IF_ENABLED_AVX512(...)
#endif

#ifdef KFR_MULTI_ENABLED_AVX2
#define KFR_IF_ENABLED_AVX2(...) __VA_ARGS__
#else
#define KFR_IF_ENABLED_AVX2(...)
#endif

#ifdef KFR_MULTI_ENABLED_AVX
#define KFR_IF_ENABLED_AVX(...) __VA_ARGS__
#else
#define KFR_IF_ENABLED_AVX(...)
#endif

#ifdef KFR_MULTI_ENABLED_SSE42
#define KFR_IF_ENABLED_SSE42(...) __VA_ARGS__
#else
#define KFR_IF_ENABLED_SSE42(...)
#endif

#ifdef KFR_MULTI_ENABLED_SSE41
#define KFR_IF_ENABLED_SSE41(...) __VA_ARGS__
#else
#define KFR_IF_ENABLED_SSE41(...)
#endif

#ifdef KFR_MULTI_ENABLED_SSSE3
#define KFR_IF_ENABLED_SSSE3(...) __VA_ARGS__
#else
#define KFR_IF_ENABLED_SSSE3(...)
#endif

#ifdef KFR_MULTI_ENABLED_SSE3
#define KFR_IF_ENABLED_SSE3(...) __VA_ARGS__
#else
#define KFR_IF_ENABLED_SSE3(...)
#endif

#ifdef KFR_MULTI_ENABLED_SSE2
#define KFR_IF_ENABLED_SSE2(...) __VA_ARGS__
#else
#define KFR_IF_ENABLED_SSE2(...)
#endif

#ifdef KFR_MULTI

#define KFR_MULTI_PROTO_GATE(...)                                                                            \
    if (cpu == cpu_t::runtime)                                                                               \
        cpu = get_cpu();                                                                                     \
    switch (cpu)                                                                                             \
    {                                                                                                        \
    case cpu_t::avx512:                                                                                      \
        KFR_IF_ENABLED_AVX512(return avx512::__VA_ARGS__;)                                                   \
    case cpu_t::avx2:                                                                                        \
        KFR_IF_ENABLED_AVX2(return avx2::__VA_ARGS__;)                                                       \
    case cpu_t::avx:                                                                                         \
        KFR_IF_ENABLED_AVX(return avx::__VA_ARGS__;)                                                         \
    case cpu_t::sse42:                                                                                       \
        KFR_IF_ENABLED_SSE42(return sse42::__VA_ARGS__;)                                                     \
    case cpu_t::sse41:                                                                                       \
        KFR_IF_ENABLED_SSE41(return sse41::__VA_ARGS__;)                                                     \
    case cpu_t::ssse3:                                                                                       \
        KFR_IF_ENABLED_SSSE3(return ssse3::__VA_ARGS__;)                                                     \
    case cpu_t::sse3:                                                                                        \
        KFR_IF_ENABLED_SSE3(return sse3::__VA_ARGS__;)                                                       \
    case cpu_t::sse2:                                                                                        \
        KFR_IF_ENABLED_SSE2(return sse2::__VA_ARGS__;)                                                       \
    default:                                                                                                 \
        KFR_UNREACHABLE;                                                                                     \
    }

#define KFR_MULTI_GATE(...)                                                                                  \
    switch (get_cpu())                                                                                       \
    {                                                                                                        \
    case cpu_t::avx512:                                                                                      \
        KFR_IF_ENABLED_AVX512({                                                                              \
            namespace ns = kfr::avx512;                                                                      \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::avx2:                                                                                        \
        KFR_IF_ENABLED_AVX2({                                                                                \
            namespace ns = kfr::avx2;                                                                        \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::avx:                                                                                         \
        KFR_IF_ENABLED_AVX({                                                                                 \
            namespace ns = kfr::avx;                                                                         \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::sse42:                                                                                       \
        KFR_IF_ENABLED_SSE42({                                                                               \
            namespace ns = kfr::sse42;                                                                       \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::sse41:                                                                                       \
        KFR_IF_ENABLED_SSE41({                                                                               \
            namespace ns = kfr::sse41;                                                                       \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::ssse3:                                                                                       \
        KFR_IF_ENABLED_SSSE3({                                                                               \
            namespace ns = kfr::ssse3;                                                                       \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::sse3:                                                                                        \
        KFR_IF_ENABLED_SSE3({                                                                                \
            namespace ns = kfr::sse3;                                                                        \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    case cpu_t::sse2:                                                                                        \
        KFR_IF_ENABLED_SSE2({                                                                                \
            namespace ns = kfr::sse2;                                                                        \
            __VA_ARGS__;                                                                                     \
            break;                                                                                           \
        })                                                                                                   \
    default:                                                                                                 \
        KFR_UNREACHABLE;                                                                                     \
    }

#define KFR_MULTI_PROTO(...)                                                                                 \
    KFR_IF_ENABLED_SSE2(KFR_IF_IS_SSE2(inline) namespace sse2{ __VA_ARGS__ })                                \
    KFR_IF_ENABLED_SSE3(KFR_IF_IS_SSE3(inline) namespace sse3{ __VA_ARGS__ })                                \
    KFR_IF_ENABLED_SSSE3(KFR_IF_IS_SSSE3(inline) namespace ssse3{ __VA_ARGS__ })                             \
    KFR_IF_ENABLED_SSE42(KFR_IF_IS_SSE42(inline) namespace sse42{ __VA_ARGS__ })                             \
    KFR_IF_ENABLED_SSE41(KFR_IF_IS_SSE41(inline) namespace sse41{ __VA_ARGS__ })                             \
    KFR_IF_ENABLED_AVX(KFR_IF_IS_AVX(inline) namespace avx{ __VA_ARGS__ })                                   \
    KFR_IF_ENABLED_AVX2(KFR_IF_IS_AVX2(inline) namespace avx2{ __VA_ARGS__ })                                \
    KFR_IF_ENABLED_AVX512(KFR_IF_IS_AVX512(inline) namespace avx512{ __VA_ARGS__ })
#else
#define KFR_MULTI_GATE(...)                                                                                  \
    do                                                                                                       \
    {                                                                                                        \
        namespace ns = kfr::KFR_ARCH_NAME;                                                                   \
        __VA_ARGS__;                                                                                         \
        break;                                                                                               \
    } while (0)

#define KFR_MULTI_PROTO(...)                                                                                 \
    inline namespace KFR_ARCH_NAME                                                                           \
    {                                                                                                        \
    __VA_ARGS__                                                                                              \
    }
#endif

#if defined(KFR_BASE_ARCH) || !defined(KFR_MULTI)
#define KFR_MULTI_NEEDS_GATE
#else
#endif

#else

// ARM

#define KFR_MULTI_PROTO_GATE(...)                                                                            \
    do                                                                                                       \
    {                                                                                                        \
        return KFR_ARCH_NAME::__VA_ARGS__;                                                                   \
    } while (0)

#define KFR_MULTI_GATE(...)                                                                                  \
    do                                                                                                       \
    {                                                                                                        \
        namespace ns = kfr::KFR_ARCH_NAME;                                                                   \
        __VA_ARGS__;                                                                                         \
        break;                                                                                               \
    } while (0)

#define KFR_MULTI_PROTO(...)                                                                                 \
    inline namespace KFR_ARCH_NAME                                                                           \
    {                                                                                                        \
    __VA_ARGS__                                                                                              \
    }

#if defined(KFR_BASE_ARCH) || !defined(KFR_MULTI)
#define KFR_MULTI_NEEDS_GATE
#else
#endif

#endif
