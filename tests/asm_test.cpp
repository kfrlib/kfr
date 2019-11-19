/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#define KFR_EXTENDED_TESTS

#include <kfr/base.hpp>
#include <kfr/dft/impl/fft-impl.hpp>
#include <kfr/io.hpp>
#include <kfr/testo/console_colors.hpp>

using namespace kfr;

#ifdef CMT_COMPILER_MSVC
#define KFR_PUBLIC CMT_PUBLIC_C CMT_DLL_EXPORT
#else
#define KFR_PUBLIC CMT_PUBLIC_C
#endif

#define TEST_ASM_8(fn, ty, MACRO)                                                                            \
    MACRO(fn, ty, 1)                                                                                         \
    MACRO(fn, ty, 2)                                                                                         \
    MACRO(fn, ty, 4)                                                                                         \
    MACRO(fn, ty, 8)                                                                                         \
    MACRO(fn, ty, 16)                                                                                        \
    MACRO(fn, ty, 32)                                                                                        \
    MACRO(fn, ty, 64)

#define TEST_ASM_16(fn, ty, MACRO)                                                                           \
    MACRO(fn, ty, 1)                                                                                         \
    MACRO(fn, ty, 2)                                                                                         \
    MACRO(fn, ty, 4)                                                                                         \
    MACRO(fn, ty, 8)                                                                                         \
    MACRO(fn, ty, 16)                                                                                        \
    MACRO(fn, ty, 32)                                                                                        \
    MACRO(fn, ty, 64)

#define TEST_ASM_32(fn, ty, MACRO)                                                                           \
    MACRO(fn, ty, 1)                                                                                         \
    MACRO(fn, ty, 2)                                                                                         \
    MACRO(fn, ty, 4)                                                                                         \
    MACRO(fn, ty, 8)                                                                                         \
    MACRO(fn, ty, 16)                                                                                        \
    MACRO(fn, ty, 32)

#define TEST_ASM_64(fn, ty, MACRO)                                                                           \
    MACRO(fn, ty, 1)                                                                                         \
    MACRO(fn, ty, 2)                                                                                         \
    MACRO(fn, ty, 4)                                                                                         \
    MACRO(fn, ty, 8)                                                                                         \
    MACRO(fn, ty, 16)

#define TEST_ASM_VTY1(fn, ty, n)                                                                             \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n>& r, const vec<ty, n>& x) { r = kfr::fn(x); }

#define TEST_ASM_VTY1_F(fn, ty, n)                                                                           \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<flt_type<ty>, n>& r, const vec<ty, n>& x)             \
    {                                                                                                        \
        r = kfr::fn(x);                                                                                      \
    }

#define TEST_ASM_VTY2(fn, ty, n)                                                                             \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n>& r, const vec<ty, n>& x, const vec<ty, n>& y)  \
    {                                                                                                        \
        r = kfr::fn(x, y);                                                                                   \
    }                                                                                                        \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n##__scalar(vec<ty, n>& r, const vec<ty, n>& x,             \
                                                             const ty& y)                                    \
    {                                                                                                        \
        r = kfr::fn(x, y);                                                                                   \
    }
#define TEST_ASM_CMP(fn, ty, n)                                                                              \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(mask<ty, n>& r, const vec<ty, n>& x, const vec<ty, n>& y) \
    {                                                                                                        \
        r = kfr::fn(x, y);                                                                                   \
    }
#define TEST_ASM_SHIFT(fn, ty, n)                                                                            \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n>& r, const vec<ty, n>& x,                       \
                                                   const vec<utype<ty>, n>& y)                               \
    {                                                                                                        \
        r = kfr::fn(x, y);                                                                                   \
    }
#define TEST_ASM_SHIFT_SCALAR(fn, ty, n)                                                                     \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n##__scalar(vec<ty, n>& r, const vec<ty, n>& x, unsigned y) \
    {                                                                                                        \
        r = kfr::fn(x, y);                                                                                   \
    }
#define TEST_ASM_VTY3(fn, ty, n)                                                                             \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n>& r, const vec<ty, n>& x, const vec<ty, n>& y,  \
                                                   const vec<ty, n>& z)                                      \
    {                                                                                                        \
        r = kfr::fn(x, y, z);                                                                                \
    }

#define GEN_ty(n, ty) ty(n)
#define GEN_arg_def(n, ty) ty arg##n
#define GEN_arg(n, ty) arg##n

#define TEST_ASM_MAKE_VECTOR(fn, ty, n)                                                                      \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n>& r, CMT_GEN_LIST(n, GEN_arg_def, ty))          \
    {                                                                                                        \
        r = kfr::fn(CMT_GEN_LIST(n, GEN_arg, ty));                                                           \
    }                                                                                                        \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n##__imm(vec<ty, n>& r)                                     \
    {                                                                                                        \
        r = kfr::fn(CMT_GEN_LIST(n, GEN_ty, ty));                                                            \
    }

#define TEST_ASM_BROADCAST(fn, ty, n)                                                                        \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n>& r, ty x) { r = kfr::fn<n>(x); }

#define TEST_ASM_HALF1(fn, ty, n)                                                                            \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n>& r, const vec<ty, n * 2>& x) { r = kfr::fn(x); }

#define TEST_ASM_DOUBLE2(fn, ty, n)                                                                          \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n * 2>& r, const vec<ty, n>& x,                   \
                                                   const vec<ty, n>& y)                                      \
    {                                                                                                        \
        r = kfr::fn(x, y);                                                                                   \
    }
#define TEST_ASM_QUAD4(fn, ty, n)                                                                            \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n * 4>& r, const vec<ty, n>& x,                   \
                                                   const vec<ty, n>& y, const vec<ty, n>& z,                 \
                                                   const vec<ty, n>& w)                                      \
    {                                                                                                        \
        r = kfr::fn(x, y, z, w);                                                                             \
    }

#define TEST_ASM_DOUBLE1(fn, ty, n)                                                                          \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n * 2>& r, const vec<ty, n>& x) { r = kfr::fn(x); }

#define TEST_READ(fn, ty, n)                                                                                 \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n##__aligned(vec<ty, n>& __restrict r,                      \
                                                              const ty* __restrict x)                        \
    {                                                                                                        \
        r = kfr::fn<n, true>(x);                                                                             \
    }                                                                                                        \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n##__unaligned(vec<ty, n> & __restrict r,                   \
                                                                const ty* __restrict x)                      \
    {                                                                                                        \
        r = kfr::fn<n, false>(x);                                                                            \
    }

#define TEST_WRITE(fn, ty, n)                                                                                \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n##__aligned(ty* __restrict p, const vec<ty, n>& x)         \
    {                                                                                                        \
        kfr::fn<true>(p, x);                                                                                 \
    }                                                                                                        \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n##__unaligned(ty * __restrict p, const vec<ty, n>& x)      \
    {                                                                                                        \
        kfr::fn<false>(p, x);                                                                                \
    }

#define TEST_ASM_U(fn, MACRO)                                                                                \
    TEST_ASM_8(fn, u8, MACRO)                                                                                \
    TEST_ASM_16(fn, u16, MACRO)                                                                              \
    TEST_ASM_32(fn, u32, MACRO)                                                                              \
    TEST_ASM_64(fn, u64, MACRO)

#define TEST_ASM_I(fn, MACRO)                                                                                \
    TEST_ASM_8(fn, i8, MACRO)                                                                                \
    TEST_ASM_16(fn, i16, MACRO)                                                                              \
    TEST_ASM_32(fn, i32, MACRO)                                                                              \
    TEST_ASM_64(fn, i64, MACRO)

#define TEST_ASM_F(fn, MACRO)                                                                                \
    TEST_ASM_32(fn, f32, MACRO)                                                                              \
    TEST_ASM_64(fn, f64, MACRO)

#define TEST_ASM_UI(fn, MACRO) TEST_ASM_U(fn, MACRO) TEST_ASM_I(fn, MACRO)

#define TEST_ASM_UIF(fn, MACRO) TEST_ASM_U(fn, MACRO) TEST_ASM_I(fn, MACRO) TEST_ASM_F(fn, MACRO)

#define TEST_ASM_IF(fn, MACRO) TEST_ASM_I(fn, MACRO) TEST_ASM_F(fn, MACRO)

#if 1

TEST_ASM_UIF(add, TEST_ASM_VTY2)

TEST_ASM_UIF(sub, TEST_ASM_VTY2)

TEST_ASM_UIF(mul, TEST_ASM_VTY2)

TEST_ASM_UIF(bitwiseand, TEST_ASM_VTY2)

TEST_ASM_UIF(equal, TEST_ASM_CMP)

TEST_ASM_IF(abs, TEST_ASM_VTY1)

TEST_ASM_IF(sqrt, TEST_ASM_VTY1_F)

TEST_ASM_IF(neg, TEST_ASM_VTY1)

TEST_ASM_UIF(bitwisenot, TEST_ASM_VTY1)

TEST_ASM_UIF(div, TEST_ASM_VTY2)

TEST_ASM_UIF(bitwiseor, TEST_ASM_VTY2)

TEST_ASM_UIF(bitwisexor, TEST_ASM_VTY2)

TEST_ASM_UIF(notequal, TEST_ASM_CMP)

TEST_ASM_UIF(less, TEST_ASM_CMP)

TEST_ASM_UIF(greater, TEST_ASM_CMP)

TEST_ASM_UIF(lessorequal, TEST_ASM_CMP)

TEST_ASM_UIF(greaterorequal, TEST_ASM_CMP)

TEST_ASM_UIF(low, TEST_ASM_HALF1)

TEST_ASM_UIF(high, TEST_ASM_HALF1)

TEST_ASM_UIF(concat, TEST_ASM_DOUBLE2)

template <typename... Args>
KFR_INTRINSIC decltype(auto) concat4(const Args&... args)
{
    return concat(args...);
}

TEST_ASM_UIF(concat4, TEST_ASM_QUAD4)

TEST_ASM_UIF(shl, TEST_ASM_SHIFT)

TEST_ASM_UIF(shr, TEST_ASM_SHIFT)

TEST_ASM_UIF(shl, TEST_ASM_SHIFT_SCALAR)

TEST_ASM_UIF(shr, TEST_ASM_SHIFT_SCALAR)

TEST_ASM_UIF(duphalfs, TEST_ASM_DOUBLE1)

TEST_ASM_UIF(sqr, TEST_ASM_VTY1)

TEST_ASM_UIF(make_vector, TEST_ASM_MAKE_VECTOR)

TEST_ASM_UIF(broadcast, TEST_ASM_BROADCAST)

TEST_ASM_UIF(read, TEST_READ)

TEST_ASM_UIF(write, TEST_WRITE)

#define TEST_FFT_SPEC(ty, size)                                                                                   \
    static intrinsics::fft_specialization<ty, size> fft__##ty##__##size(static_cast<size_t>(1 << size));     \
    KFR_PUBLIC void asm__test__fft__##ty##__##size(complex<ty>* out, const complex<ty>* in, u8* temp)        \
    {                                                                                                        \
        fft__##ty##__##size.do_execute<false>(out, in, temp);                                                \
    }                                                                                                        \
    KFR_PUBLIC void asm__test__ifft__##ty##__##size(complex<ty>* out, const complex<ty>* in, u8* temp)       \
    {                                                                                                        \
        fft__##ty##__##size.do_execute<true>(out, in, temp);                                                 \
    }
#define TEST_FFT_GEN(ty)                                                                                   \
    static intrinsics::fft_stage_impl<ty, true, true> fft__##ty##__##size(static_cast<size_t>(65526));     \
    KFR_PUBLIC void asm__test__fft__##ty##__gen(complex<ty>* out, const complex<ty>* in, u8* temp)        \
    {                                                                                                        \
        fft__##ty##__##size.do_execute<false>(out, in, temp);                                                \
    }                                                                                                        \
    KFR_PUBLIC void asm__test__ifft__##ty##__gen(complex<ty>* out, const complex<ty>* in, u8* temp)       \
    {                                                                                                        \
        fft__##ty##__##size.do_execute<true>(out, in, temp);                                                 \
    }

TEST_FFT_SPEC(f32, 1)
TEST_FFT_SPEC(f32, 2)
TEST_FFT_SPEC(f32, 3)
TEST_FFT_SPEC(f32, 4)
TEST_FFT_SPEC(f64, 1)
TEST_FFT_SPEC(f64, 2)
TEST_FFT_SPEC(f64, 3)
TEST_FFT_SPEC(f64, 4)

TEST_FFT_GEN(f32)
TEST_FFT_GEN(f64)

#endif

TEST_ASM_F(sin, TEST_ASM_VTY1_F)

TEST_ASM_F(cos, TEST_ASM_VTY1_F)

namespace kfr
{

#ifdef KFR_SHOW_NOT_OPTIMIZED
KFR_PUBLIC void not_optimized(const char* fn) CMT_NOEXCEPT { puts(fn); }
#endif
} // namespace kfr

KFR_PUBLIC void test_shuffle_old1(f32x1& x, const f32x4& y)
{
    x.v = kfr::intrinsics::simd_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v, csizes<2>, overload_auto);
}

KFR_PUBLIC void test_shuffle_old2(f32x4& x, const f32x4& y)
{
    x.v = kfr::intrinsics::simd_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v, csizes<3, 2, 1, 0>,
                                        overload_auto);
}

KFR_PUBLIC void test_shuffle_old3(f32x4& x, const f32x4& y)
{
    x.v = kfr::intrinsics::simd_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v, csizes<0, 1, 2, 3>,
                                        overload_auto);
}

KFR_PUBLIC void test_shuffle_old4(f32x2& x, const f32x4& y)
{
    x.v = kfr::intrinsics::simd_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v, csizes<2, 3>, overload_auto);
}

KFR_PUBLIC void test_shuffle_old5(f32x8& x, const f32x4& y)
{
    x.v = kfr::intrinsics::simd_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v,
                                        csizes<3, 2, 1, 0, 0, 1, 2, 3>, overload_auto);
}

KFR_PUBLIC void test_shuffle_old6(f32x8& x, const f32x4& y)
{
    x.v = kfr::intrinsics::simd_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v,
                                        csizes<7, 6, 5, 4, 3, 2, 1, 0>, overload_auto);
}

KFR_PUBLIC void test_shuffle_old9(vec<f32, 3>& x, const vec<f32, 15>& y)
{
    x.v = kfr::intrinsics::simd_shuffle(kfr::intrinsics::simd_t<f32, 15>{}, y.v, csizes<3, 2, 1>,
                                        overload_auto);
}

KFR_PUBLIC void test_shuffle_new1(f32x1& x, const f32x4& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v, csizes<2>);
}

KFR_PUBLIC void test_shuffle_new2(f32x4& x, const f32x4& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v, csizes<3, 2, 1, 0>);
}

KFR_PUBLIC void test_shuffle_new3(f32x4& x, const f32x4& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v, csizes<0, 1, 2, 3>);
}

KFR_PUBLIC void test_shuffle_new4(f32x2& x, const f32x4& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v, csizes<2, 3>);
}

KFR_PUBLIC void test_shuffle_new5(f32x8& x, const f32x4& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v,
                                             csizes<3, 2, 1, 0, 0, 1, 2, 3>);
}

KFR_PUBLIC void test_shuffle_new6(f32x8& x, const f32x4& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 4>{}, y.v,
                                             csizes<7, 6, 5, 4, 3, 2, 1, 0>);
}

KFR_PUBLIC void test_shuffle_new7(f32x1& x, const f32x32& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 32>{}, (y + 1.f).v, csizes<19>);
}

KFR_PUBLIC void test_shuffle_new8(f32x8& x, const f32x8& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 8>{}, y.v,
                                             csizes<3, 2, 1, 0, 3, 2, 1, 0>);
}

KFR_PUBLIC void test_shuffle_new9(vec<f32, 3>& x, const vec<f32, 15>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 15>{}, y.v, csizes<3, 2, 1>);
}

KFR_PUBLIC void test_shuffle_new9a(vec<f32, 3>& x, const vec<f32, 15>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 15>{}, y.v, csizes<5, 6, 7>);
}

KFR_PUBLIC void test_shuffle_new9b(vec<f32, 3>& x, const vec<f32, 15>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 15>{}, y.v, csizes<11, 11, 11>);
}

KFR_PUBLIC void test_shuffle_new9c(vec<f32, 3>& x, const vec<f32, 15>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 15>{}, y.v, csizes<3, 4, 5>);
}

KFR_PUBLIC void test_shuffle_new10(vec<f32, 15>& x)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 1>{}, 0.f,
                                             csizes<1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15>);
}
KFR_PUBLIC void test_shuffle_new11(vec<f32, 15>& x, float y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 1>{}, y,
                                             csizes<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0>);
}
KFR_PUBLIC void test_shuffle_new12(vec<f32, 32>& x, const vec<f32, 32>& y)
{
    x.v =
        kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 32>{}, y.v, csizeseq<32> ^ csize<1>);
}
KFR_PUBLIC void test_shuffle_new13(vec<f32, 8>& x, const vec<f32, 8>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 8>{}, y.v,
                                             csizes<0, 2, 4, 6, 1, 3, 5, 7>);
}
KFR_PUBLIC void test_shuffle_new14(vec<f32, 8>& x, const vec<f32, 8>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 8>{}, y.v,
                                             csizes<0, 4, 1, 5, 2, 6, 3, 7>);
}
KFR_PUBLIC void test_shuffle_new15(vec<f32, 4>& x, const vec<f32, 8>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 8>{}, y.v, csizes<0, 5, 2, 7>);
}
KFR_PUBLIC void test_shuffle_new16(vec<f32, 2>& x, const vec<f32, 2>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 2>{}, y.v, csizes<1, 0>);
}
KFR_PUBLIC void test_shuffle_new17(vec<f32, 16>& x, const vec<f32, 16>& y)
{
    x.v = kfr::intrinsics::universal_shuffle(kfr::intrinsics::simd_t<f32, 16>{}, y.v,
                                             csizes<0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15>);
}

KFR_PUBLIC float tuple_assign()
{
    auto [x, y, z, w] = f32x4(1.f, 2.f, 3.f, 4.f);
    return x + y * y + z * z * z + w * w * w * w;
}

int main() { println(library_version()); }
