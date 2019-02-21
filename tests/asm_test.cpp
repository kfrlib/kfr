/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/io.hpp>
#include <kfr/testo/console_colors.hpp>

using namespace kfr;

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

#ifdef CMT_COMPILER_MSVC
#define KFR_PUBLIC CMT_PUBLIC_C CMT_DLL_EXPORT
#else
#define KFR_PUBLIC CMT_PUBLIC_C
#endif

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

#define TEST_ASM_DOUBLE1(fn, ty, n)                                                                          \
    KFR_PUBLIC void asm__test__##fn##__##ty##__##n(vec<ty, n * 2>& r, const vec<ty, n>& x) { r = kfr::fn(x); }

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

TEST_ASM_UIF(shl, TEST_ASM_SHIFT)

TEST_ASM_UIF(shr, TEST_ASM_SHIFT)

TEST_ASM_UIF(shl, TEST_ASM_SHIFT_SCALAR)

TEST_ASM_UIF(shr, TEST_ASM_SHIFT_SCALAR)

TEST_ASM_UIF(duphalfs, TEST_ASM_DOUBLE1)

TEST_ASM_F(sin, TEST_ASM_VTY1_F)

TEST_ASM_F(cos, TEST_ASM_VTY1_F)

TEST_ASM_UIF(sqr, TEST_ASM_VTY1)

TEST_ASM_UIF(make_vector, TEST_ASM_MAKE_VECTOR)

TEST_ASM_UIF(broadcast, TEST_ASM_BROADCAST)

namespace kfr
{
#ifdef KFR_SHOW_NOT_OPTIMIZED
CMT_PUBLIC_C CMT_DLL_EXPORT void not_optimized(const char* fn) CMT_NOEXCEPT { puts(fn); }
#endif
} // namespace kfr

int main() { println(library_version()); }
