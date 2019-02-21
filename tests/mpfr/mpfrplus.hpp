#pragma once

#define MPFR_STRING(str) #str

#if defined __clang__
#define MPFR_DIAG_PRAGMA(pragma) _Pragma(MPFR_STRING(clang diagnostic pragma))
#else
#define MPFR_DIAG_PRAGMA(pragma)
#endif

MPFR_DIAG_PRAGMA(push)
MPFR_DIAG_PRAGMA(ignored "-Wreserved-id-macro")
MPFR_DIAG_PRAGMA(ignored "-Wpadded")
MPFR_DIAG_PRAGMA(ignored "-Wdeprecated")
MPFR_DIAG_PRAGMA(ignored "-Wshorten-64-to-32")
MPFR_DIAG_PRAGMA(ignored "-Wsign-conversion")
#include "mpfr.h"
MPFR_DIAG_PRAGMA(pop)
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>

namespace mpfr
{

/// Rounding mode values
enum rounding_mode
{
    nearest        = MPFR_RNDN, // round to nearest, with ties to even
    toward_zero    = MPFR_RNDZ, // round toward zero
    toward_pinf    = MPFR_RNDU, // round toward +Inf
    toward_ninf    = MPFR_RNDD, // round toward -Inf
    away_from_zero = MPFR_RNDA // round away from zero
};

constexpr int extra_precision = 5;

namespace internal
{

struct with_precision_t
{
};

constexpr with_precision_t with_precision{};
} // namespace internal

namespace internal
{
inline mpfr_prec_t& precision()
{
    static mpfr_prec_t prec = mpfr_get_default_prec();
    return prec;
}
inline mpfr_rnd_t& rounding_mode()
{
    static mpfr_rnd_t rnd = mpfr_get_default_rounding_mode();
    return rnd;
}
} // namespace internal

/// Temporarily sets the precision
struct scoped_precision
{
    inline scoped_precision(mpfr_prec_t prec)
    {
        saved_precision       = internal::precision();
        internal::precision() = prec;
    }
    inline ~scoped_precision() { internal::precision() = saved_precision; }
    mpfr_prec_t saved_precision;
};

/// Temporarily sets the rounding mode
struct scoped_rounding_mode
{
    inline scoped_rounding_mode(mpfr_rnd_t rnd)
    {
        saved_rounding_mode       = internal::rounding_mode();
        internal::rounding_mode() = rnd;
    }
    inline scoped_rounding_mode(rounding_mode rnd)
    {
        saved_rounding_mode       = internal::rounding_mode();
        internal::rounding_mode() = static_cast<mpfr_rnd_t>(rnd);
    }
    inline ~scoped_rounding_mode() { internal::rounding_mode() = saved_rounding_mode; }
    mpfr_rnd_t saved_rounding_mode;
};

#define MPFR_MACRO_CONCAT2(x, y) x##y
#define MPFR_MACRO_CONCAT(x, y) MPFR_MACRO_CONCAT2(x, y)

#define MPFR_CXX_ROUNDING_MODE(rnd)                                                                          \
    MPFR_MACRO_CONCAT(::mpfr::scoped_rounding_mode rounding_mode, __COUNTER__)(rnd)
#define MPFR_CXX_PRECISION(prec) MPFR_MACRO_CONCAT(::mpfr::scoped_precision precision, __COUNTER__)(prec)

#define MPFR_FN(fn)                                                                                          \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        inline decltype(fn(std::declval<Args>()...)) operator()(Args&&... args) const                        \
        {                                                                                                    \
            return fn(std::forward<Args>(args)...);                                                          \
        }                                                                                                    \
    };

#define MPFR_CXX_CMP(op, mpfr_fn)                                                                            \
    inline bool operator op(const number& x, const number& y)                                                \
    {                                                                                                        \
        return mpfr_fn(x.mpfr_val(), y.mpfr_val()) op 0;                                                     \
    }
#define MPFR_CXX_CMP_RT(op, mpfr_fn, type)                                                                   \
    inline bool operator op(const number& x, type y) { return mpfr_fn(x.mpfr_val(), y) op 0; }
#define MPFR_CXX_CMP_TR(op, mpfr_fn, type)                                                                   \
    inline bool operator op(type x, const number& y) { return 0 op mpfr_fn(y.mpfr_val(), x); }
#define MPFR_CXX_UNARY(fn, mpfr_fn)                                                                          \
    inline number fn(const number& x)                                                                        \
    {                                                                                                        \
        number result;                                                                                       \
        mpfr_fn(result.mpfr_val(), x.mpfr_val(), internal::rounding_mode());                                 \
        return result;                                                                                       \
    }
#define MPFR_CXX_UNARY_RND(fn, mpfr_fn, rnd)                                                                 \
    inline number fn(const number& x)                                                                        \
    {                                                                                                        \
        number result;                                                                                       \
        mpfr_fn(result.mpfr_val(), x.mpfr_val() rnd);                                                        \
        return result;                                                                                       \
    }
#define MPFR_CXX_BINARY(fn, mpfr_fn)                                                                         \
    inline number fn(const number& x, const number& y)                                                       \
    {                                                                                                        \
        number result;                                                                                       \
        mpfr_fn(result.mpfr_val(), x.mpfr_val(), y.mpfr_val(), internal::rounding_mode());                   \
        return result;                                                                                       \
    }
#define MPFR_CXX_TERNARY(fn, mpfr_fn)                                                                        \
    inline number fn(const number& x, const number& y, const number& z)                                      \
    {                                                                                                        \
        number result;                                                                                       \
        mpfr_fn(result.mpfr_val(), x.mpfr_val(), y.mpfr_val(), z.mpfr_val(), internal::rounding_mode());     \
        return result;                                                                                       \
    }
#define MPFR_CXX_BINARY_RT(fn, mpfr_fn, type)                                                                \
    inline number fn(const number& x, type y)                                                                \
    {                                                                                                        \
        number result;                                                                                       \
        mpfr_fn(result.mpfr_val(), x.mpfr_val(), y, internal::rounding_mode());                              \
        return result;                                                                                       \
    }
#define MPFR_CXX_BINARY_TR(fn, mpfr_fn, type)                                                                \
    inline number fn(type x, const number& y)                                                                \
    {                                                                                                        \
        number result;                                                                                       \
        mpfr_fn(result.mpfr_val(), x, y.mpfr_val(), internal::rounding_mode());                              \
        return result;                                                                                       \
    }
#define MPFR_CXX_INPLACE_BINARY(fn, mpfr_fn)                                                                 \
    inline number& fn(number& x, const number& y)                                                            \
    {                                                                                                        \
        mpfr_fn(x.mpfr_val(), x.mpfr_val(), y.mpfr_val(), internal::rounding_mode());                        \
        return x;                                                                                            \
    }
#define MPFR_CXX_INPLACE_BINARY_T(fn, mpfr_fn, type)                                                         \
    inline number& fn(number& x, type y)                                                                     \
    {                                                                                                        \
        mpfr_fn(x.mpfr_val(), x.mpfr_val(), y, internal::rounding_mode());                                   \
        return x;                                                                                            \
    }
#define MPFR_CXX_CONST(fn, mpfr_fn)                                                                          \
    inline number fn()                                                                                       \
    {                                                                                                        \
        number result;                                                                                       \
        mpfr_fn(result.mpfr_val(), internal::rounding_mode());                                               \
        return result;                                                                                       \
    }

/// Floating-point value with arbitrary precision
struct number
{
private:
    mpfr_t val;
    bool owns;

public:
    inline number() noexcept : owns(true) { mpfr_init2(val, internal::precision()); }
    inline ~number()
    {
        if (owns)
            mpfr_clear(val);
    }
    inline number(internal::with_precision_t, mpfr_prec_t precision) : owns(true)
    {
        mpfr_init2(val, precision);
    }
    /// Copy-constructor
    inline number(const number& value) noexcept : owns(true)
    {
        mpfr_init2(val, internal::precision());
        mpfr_set(val, value.val, internal::rounding_mode());
    }
    /// Move-constructor
    inline number(number&& value) noexcept : owns(true)
    {
        val[0]     = value.val[0];
        value.owns = false;
    }
    /**
    Construct from string
    `str`  C-string containing string representation of the number
    `base` Radix (defaults to 10)

    See also operator""_mpfr
    */
    inline number(const char* str, int base = 10) noexcept : owns(true)
    {
        mpfr_init2(val, internal::precision());
        mpfr_set_str(val, str, base, internal::rounding_mode());
    }
    inline number(std::nullptr_t) = delete;
#define MPFR_CXX_CTOR_T(mpfr_fn, type)                                                                       \
    inline number(type value) noexcept : owns(true)                                                          \
    {                                                                                                        \
        mpfr_init2(val, internal::precision());                                                              \
        mpfr_fn(val, value, internal::rounding_mode());                                                      \
    }
#define MPFR_CXX_ASGN_T(mpfr_fn, type)                                                                       \
    inline number& operator=(type value) noexcept                                                            \
    {                                                                                                        \
        mpfr_fn(val, value, internal::rounding_mode());                                                      \
        return *this;                                                                                        \
    }
    MPFR_CXX_CTOR_T(mpfr_set_d, double)
    MPFR_CXX_CTOR_T(mpfr_set_ld, long double)
    MPFR_CXX_CTOR_T(mpfr_set_flt, float)
    MPFR_CXX_CTOR_T(mpfr_set_si, int)
    MPFR_CXX_CTOR_T(mpfr_set_ui, unsigned int)
    MPFR_CXX_CTOR_T(mpfr_set_si, long int)
    MPFR_CXX_CTOR_T(mpfr_set_ui, unsigned long int)
#ifdef _MPFR_H_HAVE_INTMAX_T
    MPFR_CXX_CTOR_T(mpfr_set_sj, intmax_t)
    MPFR_CXX_CTOR_T(mpfr_set_uj, uintmax_t)
#endif

    MPFR_CXX_ASGN_T(mpfr_set_d, double)
    MPFR_CXX_ASGN_T(mpfr_set_ld, long double)
    MPFR_CXX_ASGN_T(mpfr_set_flt, float)
    MPFR_CXX_ASGN_T(mpfr_set_si, int)
    MPFR_CXX_ASGN_T(mpfr_set_ui, unsigned int)
    MPFR_CXX_ASGN_T(mpfr_set_si, long int)
    MPFR_CXX_ASGN_T(mpfr_set_ui, unsigned long int)
#ifdef _MPFR_H_HAVE_INTMAX_T
    MPFR_CXX_ASGN_T(mpfr_set_sj, intmax_t)
    MPFR_CXX_ASGN_T(mpfr_set_uj, uintmax_t)
#endif

    inline number& operator=(const number& value) noexcept
    {
        mpfr_set(val, value.val, internal::rounding_mode());
        return *this;
    }
    /// Get internal mpfr value
    inline mpfr_ptr mpfr_val() { return &val[0]; }
    inline mpfr_srcptr mpfr_val() const { return &val[0]; }

    MPFR_DIAG_PRAGMA(push)
    MPFR_DIAG_PRAGMA(ignored "-Wold-style-cast")
    /// Current precision
    inline mpfr_prec_t prec() const { return mpfr_get_prec(val); }
    /// Is this number zero?
    inline bool iszero() const { return mpfr_zero_p(val); }
    /// Is this number NaN?
    inline bool isnan() const { return mpfr_nan_p(val); }
    /// Is this number Infinity?
    inline bool isinfinity() const { return mpfr_inf_p(val); }
    /// Is this number finite?
    inline bool isfinite() const { return !isinfinity() && !isnan(); }
    /// Returns sign of the number
    inline int sign() const { return MPFR_SIGN(val); }

    MPFR_DIAG_PRAGMA(pop)
    /// Converts precision
    inline number withprec(mpfr_prec_t prec) const
    {
        number r(internal::with_precision, prec);
        r = *this;
        return r;
    }

    /// Explicit conversion to float
    inline explicit operator float() const noexcept { return mpfr_get_flt(val, internal::rounding_mode()); }
    /// Explicit conversion to double
    inline explicit operator double() const noexcept { return mpfr_get_d(val, internal::rounding_mode()); }
    /// Explicit conversion to long double
    inline explicit operator long double() const noexcept
    {
        return mpfr_get_ld(val, internal::rounding_mode());
    }

    std::string to_string() const
    {
        char* str;
        mpfr_asprintf(&str, "%.*Rg", prec(), val);
        std::string result = str;
        mpfr_free_str(str);
        return result;
    }
};

#ifdef MPFR_USE_UDL
/**
Constructs number from string using User-Defined-Literal (C++11).
#### Example:
```C++
using namespace mpfr;
number r = "1.1111111111111111111111111111111111111111"_mpfr;
```
*/
inline number operator""_mpfr(const char* str, size_t) { return str; }
/// Constructs number from long double using User-Defined-Literal (C++11)
inline number operator""_mpfr(long double val) { return val; }
#endif

MPFR_CXX_CMP(==, mpfr_cmp)
MPFR_CXX_CMP(!=, mpfr_cmp)
MPFR_CXX_CMP(<, mpfr_cmp)
MPFR_CXX_CMP(>, mpfr_cmp)
MPFR_CXX_CMP(>=, mpfr_cmp)
MPFR_CXX_CMP(<=, mpfr_cmp)
MPFR_CXX_CMP_RT(==, mpfr_cmp_si, int)
MPFR_CXX_CMP_RT(!=, mpfr_cmp_si, int)
MPFR_CXX_CMP_RT(<, mpfr_cmp_si, int)
MPFR_CXX_CMP_RT(>, mpfr_cmp_si, int)
MPFR_CXX_CMP_RT(>=, mpfr_cmp_si, int)
MPFR_CXX_CMP_RT(<=, mpfr_cmp_si, int)
MPFR_CXX_CMP_RT(==, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_RT(!=, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_RT(<, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_RT(>, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_RT(>=, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_RT(<=, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_RT(==, mpfr_cmp_si, long int)
MPFR_CXX_CMP_RT(!=, mpfr_cmp_si, long int)
MPFR_CXX_CMP_RT(<, mpfr_cmp_si, long int)
MPFR_CXX_CMP_RT(>, mpfr_cmp_si, long int)
MPFR_CXX_CMP_RT(>=, mpfr_cmp_si, long int)
MPFR_CXX_CMP_RT(<=, mpfr_cmp_si, long int)
MPFR_CXX_CMP_RT(==, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_RT(!=, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_RT(<, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_RT(>, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_RT(>=, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_RT(<=, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_RT(==, mpfr_cmp_d, double)
MPFR_CXX_CMP_RT(!=, mpfr_cmp_d, double)
MPFR_CXX_CMP_RT(<, mpfr_cmp_d, double)
MPFR_CXX_CMP_RT(>, mpfr_cmp_d, double)
MPFR_CXX_CMP_RT(>=, mpfr_cmp_d, double)
MPFR_CXX_CMP_RT(<=, mpfr_cmp_d, double)

MPFR_CXX_CMP_TR(==, mpfr_cmp_si, int)
MPFR_CXX_CMP_TR(!=, mpfr_cmp_si, int)
MPFR_CXX_CMP_TR(<, mpfr_cmp_si, int)
MPFR_CXX_CMP_TR(>, mpfr_cmp_si, int)
MPFR_CXX_CMP_TR(>=, mpfr_cmp_si, int)
MPFR_CXX_CMP_TR(<=, mpfr_cmp_si, int)
MPFR_CXX_CMP_TR(==, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_TR(!=, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_TR(<, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_TR(>, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_TR(>=, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_TR(<=, mpfr_cmp_ui, unsigned int)
MPFR_CXX_CMP_TR(==, mpfr_cmp_si, long int)
MPFR_CXX_CMP_TR(!=, mpfr_cmp_si, long int)
MPFR_CXX_CMP_TR(<, mpfr_cmp_si, long int)
MPFR_CXX_CMP_TR(>, mpfr_cmp_si, long int)
MPFR_CXX_CMP_TR(>=, mpfr_cmp_si, long int)
MPFR_CXX_CMP_TR(<=, mpfr_cmp_si, long int)
MPFR_CXX_CMP_TR(==, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_TR(!=, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_TR(<, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_TR(>, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_TR(>=, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_TR(<=, mpfr_cmp_ui, unsigned long int)
MPFR_CXX_CMP_TR(==, mpfr_cmp_d, double)
MPFR_CXX_CMP_TR(!=, mpfr_cmp_d, double)
MPFR_CXX_CMP_TR(<, mpfr_cmp_d, double)
MPFR_CXX_CMP_TR(>, mpfr_cmp_d, double)
MPFR_CXX_CMP_TR(>=, mpfr_cmp_d, double)
MPFR_CXX_CMP_TR(<=, mpfr_cmp_d, double)

MPFR_CXX_UNARY(operator-, mpfr_neg)
MPFR_CXX_BINARY(operator+, mpfr_add)
MPFR_CXX_BINARY(operator-, mpfr_sub)
MPFR_CXX_BINARY(operator*, mpfr_mul)
MPFR_CXX_BINARY(operator/, mpfr_div)
MPFR_CXX_BINARY_RT(operator+, mpfr_add_si, int)
MPFR_CXX_BINARY_RT(operator-, mpfr_sub_si, int)
MPFR_CXX_BINARY_RT(operator*, mpfr_mul_si, int)
MPFR_CXX_BINARY_RT(operator/, mpfr_div_si, int)
MPFR_CXX_BINARY_RT(operator+, mpfr_add_si, long int)
MPFR_CXX_BINARY_RT(operator-, mpfr_sub_si, long int)
MPFR_CXX_BINARY_RT(operator*, mpfr_mul_si, long int)
MPFR_CXX_BINARY_RT(operator/, mpfr_div_si, long int)
MPFR_CXX_BINARY_RT(operator+, mpfr_add_ui, unsigned int)
MPFR_CXX_BINARY_RT(operator-, mpfr_sub_ui, unsigned int)
MPFR_CXX_BINARY_RT(operator*, mpfr_mul_ui, unsigned int)
MPFR_CXX_BINARY_RT(operator/, mpfr_div_ui, unsigned int)
MPFR_CXX_BINARY_RT(operator+, mpfr_add_ui, unsigned long int)
MPFR_CXX_BINARY_RT(operator-, mpfr_sub_ui, unsigned long int)
MPFR_CXX_BINARY_RT(operator*, mpfr_mul_ui, unsigned long int)
MPFR_CXX_BINARY_RT(operator/, mpfr_div_ui, unsigned long int)
MPFR_CXX_BINARY_RT(operator+, mpfr_add_d, double)
MPFR_CXX_BINARY_RT(operator-, mpfr_sub_d, double)
MPFR_CXX_BINARY_RT(operator*, mpfr_mul_d, double)
MPFR_CXX_BINARY_RT(operator/, mpfr_div_d, double)

MPFR_CXX_BINARY_TR(operator-, mpfr_si_sub, int)
MPFR_CXX_BINARY_TR(operator/, mpfr_si_div, int)
MPFR_CXX_BINARY_TR(operator-, mpfr_si_sub, long int)
MPFR_CXX_BINARY_TR(operator/, mpfr_si_div, long int)
MPFR_CXX_BINARY_TR(operator-, mpfr_ui_sub, unsigned int)
MPFR_CXX_BINARY_TR(operator/, mpfr_ui_div, unsigned int)
MPFR_CXX_BINARY_TR(operator-, mpfr_ui_sub, unsigned long int)
MPFR_CXX_BINARY_TR(operator/, mpfr_ui_div, unsigned long int)
MPFR_CXX_BINARY_TR(operator-, mpfr_d_sub, double)
MPFR_CXX_BINARY_TR(operator/, mpfr_d_div, double)

MPFR_CXX_INPLACE_BINARY(operator+=, mpfr_add)
MPFR_CXX_INPLACE_BINARY(operator-=, mpfr_sub)
MPFR_CXX_INPLACE_BINARY(operator*=, mpfr_mul)
MPFR_CXX_INPLACE_BINARY(operator/=, mpfr_div)

MPFR_CXX_INPLACE_BINARY_T(operator+=, mpfr_add_si, int)
MPFR_CXX_INPLACE_BINARY_T(operator-=, mpfr_sub_si, int)
MPFR_CXX_INPLACE_BINARY_T(operator*=, mpfr_mul_si, int)
MPFR_CXX_INPLACE_BINARY_T(operator/=, mpfr_div_si, int)
MPFR_CXX_INPLACE_BINARY_T(operator+=, mpfr_add_si, long int)
MPFR_CXX_INPLACE_BINARY_T(operator-=, mpfr_sub_si, long int)
MPFR_CXX_INPLACE_BINARY_T(operator*=, mpfr_mul_si, long int)
MPFR_CXX_INPLACE_BINARY_T(operator/=, mpfr_div_si, long int)
MPFR_CXX_INPLACE_BINARY_T(operator+=, mpfr_add_ui, unsigned int)
MPFR_CXX_INPLACE_BINARY_T(operator-=, mpfr_sub_ui, unsigned int)
MPFR_CXX_INPLACE_BINARY_T(operator*=, mpfr_mul_ui, unsigned int)
MPFR_CXX_INPLACE_BINARY_T(operator/=, mpfr_div_ui, unsigned int)
MPFR_CXX_INPLACE_BINARY_T(operator+=, mpfr_add_ui, unsigned long int)
MPFR_CXX_INPLACE_BINARY_T(operator-=, mpfr_sub_ui, unsigned long int)
MPFR_CXX_INPLACE_BINARY_T(operator*=, mpfr_mul_ui, unsigned long int)
MPFR_CXX_INPLACE_BINARY_T(operator/=, mpfr_div_ui, unsigned long int)
MPFR_CXX_INPLACE_BINARY_T(operator+=, mpfr_add_d, double)
MPFR_CXX_INPLACE_BINARY_T(operator-=, mpfr_sub_d, double)
MPFR_CXX_INPLACE_BINARY_T(operator*=, mpfr_mul_d, double)
MPFR_CXX_INPLACE_BINARY_T(operator/=, mpfr_div_d, double)

MPFR_CXX_UNARY(sqrt, mpfr_sqrt)
MPFR_CXX_UNARY(recipsqrt, mpfr_rec_sqrt)
MPFR_CXX_UNARY(cbrt, mpfr_cbrt)
MPFR_CXX_UNARY(sin, mpfr_sin)
MPFR_CXX_UNARY(cos, mpfr_cos)
MPFR_CXX_UNARY(tan, mpfr_tan)
MPFR_CXX_UNARY(cot, mpfr_cot)
MPFR_CXX_UNARY(sec, mpfr_sec)
MPFR_CXX_UNARY(csc, mpfr_csc)
MPFR_CXX_UNARY(asin, mpfr_asin)
MPFR_CXX_UNARY(acos, mpfr_acos)
MPFR_CXX_UNARY(atan, mpfr_atan)
MPFR_CXX_UNARY(sinh, mpfr_sinh)
MPFR_CXX_UNARY(cosh, mpfr_cosh)
MPFR_CXX_UNARY(tanh, mpfr_tanh)
MPFR_CXX_UNARY(coth, mpfr_coth)
MPFR_CXX_UNARY(asinh, mpfr_asinh)
MPFR_CXX_UNARY(acosh, mpfr_acosh)
MPFR_CXX_UNARY(atanh, mpfr_atanh)
MPFR_CXX_UNARY(log, mpfr_log)
MPFR_CXX_UNARY(log2, mpfr_log2)
MPFR_CXX_UNARY(log10, mpfr_log10)
MPFR_CXX_UNARY(log1p, mpfr_log1p)
MPFR_CXX_UNARY(exp, mpfr_exp)
MPFR_CXX_UNARY(exp2, mpfr_exp2)
MPFR_CXX_UNARY(exp10, mpfr_exp10)
MPFR_CXX_UNARY(expm1, mpfr_expm1)
MPFR_CXX_UNARY_RND(floor, mpfr_floor, )
MPFR_CXX_UNARY_RND(ceil, mpfr_ceil, )
MPFR_CXX_UNARY_RND(trunc, mpfr_trunc, )
MPFR_CXX_UNARY_RND(round, mpfr_round, )
MPFR_CXX_UNARY(abs, mpfr_abs)
MPFR_CXX_UNARY(erf, mpfr_erf)
MPFR_CXX_UNARY(erfc, mpfr_erfc)
MPFR_CXX_UNARY(gamma, mpfr_gamma)
MPFR_CXX_UNARY(lngamma, mpfr_lngamma)
MPFR_CXX_UNARY(digamma, mpfr_digamma)

MPFR_CXX_BINARY(min, mpfr_min)
MPFR_CXX_BINARY(max, mpfr_max)
MPFR_CXX_BINARY(dim, mpfr_dim)
MPFR_CXX_BINARY(hypot, mpfr_hypot)
MPFR_CXX_BINARY(pow, mpfr_pow)
MPFR_CXX_BINARY_RT(pow, mpfr_pow_si, int)
MPFR_CXX_BINARY_RT(pow, mpfr_pow_ui, unsigned int)
MPFR_CXX_BINARY_RT(pow, mpfr_pow_si, long int)
MPFR_CXX_BINARY_RT(pow, mpfr_pow_ui, unsigned long int)
MPFR_CXX_BINARY(atan2, mpfr_atan2)

inline number sinc(const number& x)
{
    number result;
    if (x.iszero())
    {
        return 1;
    }
    scoped_precision p(internal::precision() * 2);
    result = sin(x) / x;
    return result.withprec(internal::precision());
}

inline number logb(const number& x) { return floor(log2(x)); }

inline number pow(const number& x, double y)
{
    number result;
    number yy(y);
    mpfr_pow(result.mpfr_val(), x.mpfr_val(), yy.mpfr_val(), internal::rounding_mode());
    return result;
}

MPFR_CXX_TERNARY(fma, mpfr_fma)
MPFR_CXX_TERNARY(fms, mpfr_fms)

MPFR_CXX_CONST(const_pi, mpfr_const_pi)
MPFR_CXX_CONST(pi, mpfr_const_pi)
MPFR_CXX_CONST(const_euler, mpfr_const_euler)
MPFR_CXX_CONST(const_log2, mpfr_const_log2)
MPFR_CXX_CONST(const_catalan, mpfr_const_catalan)

inline void sincos(number& sin, number& cos, const number& x)
{
    mpfr_sin_cos(sin.mpfr_val(), cos.mpfr_val(), x.mpfr_val(), internal::rounding_mode());
}
inline void sinhcosh(number& sinh, number& cosh, const number& x)
{
    mpfr_sinh_cosh(sinh.mpfr_val(), cosh.mpfr_val(), x.mpfr_val(), internal::rounding_mode());
}

/// Return Not-a-Number
inline number nan()
{
    number result;
    mpfr_set_nan(result.mpfr_val());
    return result;
}
/// Return Infinity (positive or negative)
inline number infinity(int sign = 1)
{
    number result;
    mpfr_set_inf(result.mpfr_val(), sign);
    return result;
}
/// Return zero (positive or negative)
inline number zero(int sign = 1)
{
    number result;
    mpfr_set_zero(result.mpfr_val(), sign);
    return result;
}
/// Return n / d
inline number fraction(long n, long d)
{
    number num(n);
    return num / d;
}
/// Return 1/x
inline number reciprocal(const number& x) { return 1 / x; }

inline number horner(number /*x*/) { return zero(); }
inline number horner(number /*x*/, number c0) { return c0; }
template <typename... Ts>
inline number horner(number x, number c0, number c1, Ts... values)
{
    return fma(horner(x, c1, values...), x, c0);
}

struct complex
{
    inline complex() noexcept               = default;
    inline complex(const complex&) noexcept = default;
    inline complex(complex&&) noexcept      = default;
    inline complex& operator=(const complex&) noexcept = default;
    inline complex& operator=(complex&&) noexcept = default;
    inline complex(const number& real) noexcept : real(real), imag(zero()) {}
    inline complex(const number& real, const number& imag) noexcept : real(real), imag(imag) {}
    inline bool isreal() const { return imag.iszero(); }
    inline bool iszero() const { return real.iszero() && imag.iszero(); }
    inline bool isnan() const { return real.isnan() || imag.isnan(); }
    inline bool isinfinity() const { return real.isinfinity() || imag.isinfinity(); }
    inline bool isfinite() const { return real.isfinite() && imag.isfinite(); }
    /// Converts precision
    inline complex withprec(mpfr_prec_t prec) const { return { real.withprec(prec), imag.withprec(prec) }; }
    number real;
    number imag;
};

inline complex operator+(const complex& x, const complex& y) { return { x.real + y.real, x.imag + y.imag }; }
inline complex operator+(const complex& x, const number& y) { return { x.real + y, x.imag }; }
inline complex operator+(const number& x, const complex& y) { return y + x; }
inline complex operator-(const complex& x, const complex& y) { return { x.real - y.real, x.imag - y.imag }; }
inline complex operator-(const complex& x, const number& y) { return { x.real - y, x.imag }; }
inline complex operator-(const number& x, const complex& y) { return { x - y.real, -y.imag }; }
inline complex operator*(const complex& x, const complex& y)
{
    return { x.real * y.real - x.imag * y.imag, x.real * y.imag + x.imag * y.real };
}
inline complex operator*(const complex& x, const number& y) { return { x.real * y, x.imag * y }; }
inline complex operator*(const number& x, const complex& y) { return y * x; }

inline number abs(const complex& x) { return hypot(x.real, x.imag); }
inline number phase(const complex& x) { return atan2(x.imag, x.real); }
inline complex conj(const complex& x) { return { x.real, -x.imag }; }
inline complex reciprocal(const complex& x)
{
    number a = abs(x);
    return { x.real / a, -x.imag / a };
}
inline complex operator/(const complex& x, const complex& y) { return x * reciprocal(y); }
inline complex operator/(const complex& x, const number& y) { return x * reciprocal(y); }
inline complex operator/(const number& x, const complex& y) { return x * reciprocal(y); }

inline complex exp(const complex& x)
{
    complex sc;
    sincos(sc.imag, sc.real, x.imag);
    number e = exp(x.real);
    return { e * sc.imag, e * sc.imag };
}
inline complex sin(const complex& x)
{
    complex sc;
    complex sch;
    sincos(sc.imag, sc.real, x.real);
    sinhcosh(sch.imag, sch.real, x.imag);
    return { sc.imag * sch.real, sc.real * sch.imag };
}
inline complex cos(const complex& x)
{
    complex sc;
    complex sch;
    sincos(sc.imag, sc.real, x.real);
    sinhcosh(sch.imag, sch.real, x.imag);
    return { sc.real * sch.real, -sc.imag * sch.imag };
}
inline complex tan(const complex& x) { return sin(x) / cos(x); }

inline complex polar(const complex& x) { return { hypot(x.real, x.imag), atan2(x.imag, x.real) }; }
inline complex cartesian(const complex& x)
{
    number c, s;
    sincos(s, c, x.imag);
    return { x.real * c, x.real * s };
}

MPFR_FN(sqrt)
MPFR_FN(recipsqrt)
MPFR_FN(cbrt)
MPFR_FN(sin)
MPFR_FN(sinc)
MPFR_FN(cos)
MPFR_FN(tan)
MPFR_FN(cot)
MPFR_FN(sec)
MPFR_FN(csc)
MPFR_FN(asin)
MPFR_FN(acos)
MPFR_FN(atan)
MPFR_FN(sinh)
MPFR_FN(cosh)
MPFR_FN(tanh)
MPFR_FN(coth)
MPFR_FN(asinh)
MPFR_FN(acosh)
MPFR_FN(atanh)
MPFR_FN(log)
MPFR_FN(log2)
MPFR_FN(log10)
MPFR_FN(log1p)
MPFR_FN(logb)
MPFR_FN(exp)
MPFR_FN(exp2)
MPFR_FN(exp10)
MPFR_FN(expm1)
MPFR_FN(floor)
MPFR_FN(ceil)
MPFR_FN(trunc)
MPFR_FN(round)
MPFR_FN(abs)
MPFR_FN(erf)
MPFR_FN(erfc)
MPFR_FN(gamma)
MPFR_FN(lngamma)
MPFR_FN(digamma)
MPFR_FN(min)
MPFR_FN(max)
MPFR_FN(dim)
MPFR_FN(hypot)
MPFR_FN(pow)
MPFR_FN(atan2)
MPFR_FN(fma)
MPFR_FN(fms)
MPFR_FN(horner)
MPFR_FN(sincos)
MPFR_FN(sinhcosh)
MPFR_FN(nan)
MPFR_FN(infinity)
MPFR_FN(zero)
MPFR_FN(fraction)
MPFR_FN(reciprocal)
} // namespace mpfr

#undef MPFR_CXX_UNARY
#undef MPFR_CXX_UNARY_RND
#undef MPFR_CXX_BINARY
#undef MPFR_CXX_TERNARY
#undef MPFR_CXX_BINARY_RT
#undef MPFR_CXX_BINARY_TR
#undef MPFR_CXX_INPLACE_BINARY
#undef MPFR_CXX_INPLACE_BINARY_T
#undef MPFR_CXX_CTOR_T
#undef MPFR_CXX_ASGN_T
