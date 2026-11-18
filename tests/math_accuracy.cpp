/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/math.hpp>

extern "C"
{
#include <libbf.h>
}

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <new>
#include <random>
#include <type_traits>

namespace
{
using namespace kfr;
constexpr limb_t precision   = 256;
constexpr bf_flags_t nearest = BF_RNDN;
constexpr size_t samples     = 65536;

void check_status(int status)
{
    if (status & BF_ST_MEM_ERROR)
        throw std::bad_alloc();
}

struct context
{
    bf_context_t value;

    context()
    {
        bf_context_init(
            &value,
            [](void*, void* ptr, size_t size) -> void*
            {
                if (size != 0)
                    return std::realloc(ptr, size);
                std::free(ptr);
                return nullptr;
            },
            nullptr);
    }
    ~context() { bf_context_end(&value); }
    context(const context&)            = delete;
    context& operator=(const context&) = delete;
};

struct number
{
    bf_t value;

    explicit number(bf_context_t* ctx) { bf_init(ctx, &value); }
    ~number() { bf_delete(&value); }
    number(const number&)            = delete;
    number& operator=(const number&) = delete;

    operator bf_t*() { return &value; }
};

template <typename T>
T rounded(bf_t* x)
{
    // Round directly to the target format before converting through double.
    constexpr int exponent_bits = std::is_same_v<T, float> ? 8 : 11;
    check_status(bf_round(x, std::numeric_limits<T>::digits,
                          nearest | BF_FLAG_SUBNORMAL | bf_set_exp_bits(exponent_bits)));
    double result;
    check_status(bf_get_float64(x, &result, BF_RNDN));
    return static_cast<T>(result);
}

template <typename T>
uint64_t ulp_distance(T x, T y)
{
    using bits          = std::conditional_t<std::is_same_v<T, float>, uint32_t, uint64_t>;
    constexpr bits sign = bits(1) << (std::numeric_limits<bits>::digits - 1);
    const auto ordered  = [=](T value)
    {
        const bits raw       = std::bit_cast<bits>(value);
        const bits magnitude = raw & (sign - 1);
        return raw & sign ? sign - magnitude : sign + magnitude;
    };
    const bits a = ordered(x), b = ordered(y);
    return a > b ? a - b : b - a;
}

struct metric
{
    size_t count   = 0;
    double average = 0;
    double maximum = 0;

    void add(double value)
    {
        ++count;
        average += (value - average) / static_cast<double>(count);
        maximum = std::max(maximum, value);
    }

    void write(std::ostream& out) const
    {
        out << "{\"count\": " << count << ", \"average\": ";
        if (count)
            out << average << ", \"maximum\": " << maximum;
        else
            out << "null, \"maximum\": null";
        out << '}';
    }
};

struct statistics
{
    metric ulps, relative, absolute;
    size_t comparisons          = 0;
    size_t nonfinite_mismatches = 0;

    template <typename T>
    void compare(T actual, const bf_t* reference, bf_context_t* ctx)
    {
        ++comparisons;
        number expected(ctx);
        check_status(bf_set(expected, reference));
        const T target = rounded<T>(expected);
        if (!std::isfinite(actual) || !std::isfinite(target))
        {
            if (!(actual == target || (std::isnan(actual) && std::isnan(target))))
                ++nonfinite_mismatches;
            return;
        }
        ulps.add(static_cast<double>(ulp_distance(actual, target)));

        number delta(ctx), actual_bf(ctx), ratio(ctx);
        check_status(bf_set_float64(actual_bf, actual));
        check_status(bf_sub(delta, actual_bf, reference, precision, nearest));
        delta.value.sign = 0;
        double error;
        check_status(bf_get_float64(delta, &error, BF_RNDN));
        if (std::isfinite(error))
            absolute.add(error);
        if (!bf_is_zero(reference))
        {
            check_status(bf_div(ratio, delta, reference, precision, nearest));
            ratio.value.sign = 0;
            check_status(bf_get_float64(ratio, &error, BF_RNDN));
            if (std::isfinite(error))
                relative.add(error);
        }
    }
};

struct interval
{
    double lo, hi;

    template <typename T>
    T sample(size_t index, std::mt19937_64& rng) const
    {
        const double unit          = static_cast<double>(rng() >> 11) * 0x1p-53;
        constexpr double anchors[] = { 0, 1, 0.5, 0.25, 0.75 };
        return static_cast<T>(lo + (hi - lo) * (index < std::size(anchors) ? anchors[index] : unit));
    }
};

struct report
{
    std::ostream& out;
    bool first  = true;
    bool passed = true;

    template <typename T, size_t Arity>
    void add(const char* name, size_t lanes, const statistics& stats,
             const std::array<interval, Arity>& domains, double max_ulps, double avg_ulps)
    {
        const bool within_budget = stats.ulps.maximum <= max_ulps && stats.ulps.average <= avg_ulps;
        const bool ok            = stats.ulps.count != 0 && stats.nonfinite_mismatches == 0 && within_budget;
        if (!ok)
        {
            passed = false;
            std::cerr << name << " (" << (std::is_same_v<T, float> ? "float" : "double") << ", " << lanes
                      << " lanes): avg/max ULP " << stats.ulps.average << '/' << stats.ulps.maximum
                      << ", max absolute " << stats.absolute.maximum << ", nonfinite mismatches "
                      << stats.nonfinite_mismatches << '\n';
        }
        if (!first)
            out << ",\n";
        first = false;
        out << "    {\"function\": \"" << name << "\", \"type\": \""
            << (std::is_same_v<T, float> ? "float" : "double") << "\", \"lanes\": " << lanes
            << ", \"tested_range\": [";
        for (size_t i = 0; i < Arity; ++i)
        {
            if (i != 0)
                out << ", ";
            out << "[" << static_cast<T>(domains[i].lo) << ", " << static_cast<T>(domains[i].hi) << "]";
        }
        out << "], \"comparisons\": " << stats.comparisons
            << ", \"nonfinite_mismatches\": " << stats.nonfinite_mismatches
            << ", \"passed\": " << (ok ? "true" : "false") << ", \"ulps\": ";
        stats.ulps.write(out);
        out << ", \"relative_error\": ";
        stats.relative.write(out);
        out << ", \"absolute_error\": ";
        stats.absolute.write(out);
        out << '}';
    }
};

template <size_t Arity, typename T, typename Function, typename Reference>
void measure(report& output, bf_context_t* ctx, const char* name, const Function& function,
             const Reference& reference, std::array<interval, Arity> domains, double max_ulps,
             double avg_ulps)
{
    constexpr size_t width = std::max(size_t(2), vector_width<T>);
    using vector           = vec<T, width>;
    std::mt19937_64 rng(0x4b4652);
    number x(ctx), y(ctx), ref(ctx);
    statistics scalar, simd;
    for (size_t i = 0; i < samples; i += width)
    {
        vector xs, ys;
        for (size_t j = 0; j < width; ++j)
        {
            xs.set(j, domains[0].template sample<T>(i + j, rng));
            if constexpr (Arity == 2)
                ys.set(j, domains[1].template sample<T>(i + j + 3, rng));
        }
        const vector result = [&]
        {
            if constexpr (Arity == 1)
                return function(xs);
            else
                return function(xs, ys);
        }();
        for (size_t j = 0; j < width; ++j)
        {
            const T a = xs[j];
            check_status(bf_set_float64(x, a));
            if constexpr (Arity == 1)
            {
                check_status(reference(ref, x));
                scalar.compare(function(a), ref, ctx);
            }
            else
            {
                const T b = ys[j];
                check_status(bf_set_float64(y, b));
                check_status(reference(ref, x, y));
                scalar.compare(function(a, b), ref, ctx);
            }
            simd.compare(result[j], ref, ctx);
        }
    }
    output.add<T>(name, 1, scalar, domains, max_ulps, avg_ulps);
    output.add<T>(name, width, simd, domains, max_ulps, avg_ulps);
}

template <typename Function, typename Reference>
void unary(report& out, bf_context_t* ctx, const char* name, Function function, Reference reference,
           interval domain, double max_ulps, double avg_ulps)
{
    measure<1, float>(out, ctx, name, function, reference, { domain }, max_ulps, avg_ulps);
    measure<1, double>(out, ctx, name, function, reference, { domain }, max_ulps, avg_ulps);
}

template <typename Function, typename Reference>
void binary(report& out, bf_context_t* ctx, const char* name, Function function, Reference reference,
            interval x, interval y, double max_ulps, double avg_ulps)
{
    measure<2, float>(out, ctx, name, function, reference, { x, y }, max_ulps, avg_ulps);
    measure<2, double>(out, ctx, name, function, reference, { x, y }, max_ulps, avg_ulps);
}

auto logarithm(unsigned base)
{
    return [=](bf_t* r, const bf_t* x)
    {
        number b(r->ctx), log_b(r->ctx), log_x(r->ctx);
        int status = bf_set_ui(b, base);
        status |= bf_log(log_b, b, precision, nearest);
        status |= bf_log(log_x, x, precision, nearest);
        status |= bf_div(r, log_x, log_b, precision, nearest);
        return status;
    };
}

auto exponential(unsigned base)
{
    return [=](bf_t* r, const bf_t* x)
    {
        number b(r->ctx);
        const int status = bf_set_ui(b, base);
        return status | bf_pow(r, b, x, precision, nearest);
    };
}

int cube_root(bf_t* r, const bf_t* x)
{
    number magnitude(r->ctx), exponent(r->ctx), three(r->ctx);
    int status           = bf_set(magnitude, x);
    magnitude.value.sign = 0;
    status |= bf_set_ui(exponent, 1);
    status |= bf_set_ui(three, 3);
    status |= bf_div(exponent, exponent, three, precision, nearest);
    status |= bf_pow(r, magnitude, exponent, precision, nearest);
    r->sign = x->sign;
    return status;
}

enum class hyperbolic_function
{
    sinh,
    cosh,
    tanh,
    coth
};

auto hyperbolic(hyperbolic_function fn)
{
    return [=](bf_t* r, const bf_t* x)
    {
        number neg(r->ctx), e(r->ctx), inv(r->ctx), sum(r->ctx), diff(r->ctx);
        int status = bf_set(neg, x);
        bf_neg(neg);
        status |= bf_exp(e, x, precision, nearest);
        status |= bf_exp(inv, neg, precision, nearest);
        status |= bf_add(sum, e, inv, precision, nearest);
        status |= bf_sub(diff, e, inv, precision, nearest);
        if (fn == hyperbolic_function::tanh)
            return status | bf_div(r, diff, sum, precision, nearest);
        if (fn == hyperbolic_function::coth)
            return status | bf_div(r, sum, diff, precision, nearest);
        status |=
            bf_set(r, fn == hyperbolic_function::sinh ? static_cast<bf_t*>(diff) : static_cast<bf_t*>(sum));
        return status | bf_mul_2exp(r, -1, precision, nearest);
    };
}

void run(report& out, bf_context_t* ctx)
{
#define KFR_ACCURACY_UNARY(fn, lo, hi, peak, avg)                                                            \
    unary(                                                                                                   \
        out, ctx, #fn, [](auto x) { return kfr::fn(x); },                                                    \
        [](bf_t* r, const bf_t* x) { return bf_##fn(r, x, precision, nearest); }, { lo, hi }, peak, avg)
    KFR_ACCURACY_UNARY(sqrt, 0, 1000, 1, 1);
    KFR_ACCURACY_UNARY(exp, -10, 10, 4, 2);
    KFR_ACCURACY_UNARY(log, 0.125, 100, 3, 1);
    KFR_ACCURACY_UNARY(sin, -6.28, 6.28, 7, 1);
    KFR_ACCURACY_UNARY(cos, -6.28, 6.28, 7, 1);
    KFR_ACCURACY_UNARY(tan, -3.14, 3.14, 10, 1);
    KFR_ACCURACY_UNARY(asin, -1, 1, 6, 1);
    KFR_ACCURACY_UNARY(acos, -1, 1, 8, 1);
    KFR_ACCURACY_UNARY(atan, -10, 10, 3, 1);
#undef KFR_ACCURACY_UNARY

#define KFR_ACCURACY_DERIVED(fn, ref, lo, hi, peak, avg)                                                     \
    unary(out, ctx, #fn, [](auto x) { return kfr::fn(x); }, ref, { lo, hi }, peak, avg)
    KFR_ACCURACY_DERIVED(exp2, exponential(2), -10, 10, 8, 2);
    KFR_ACCURACY_DERIVED(exp10, exponential(10), -10, 10, 48, 10);
    KFR_ACCURACY_DERIVED(log2, logarithm(2), 0.125, 100, 4, 1);
    KFR_ACCURACY_DERIVED(log10, logarithm(10), 0.125, 100, 4, 1);
    KFR_ACCURACY_DERIVED(cbrt, cube_root, -1000, 1000, 8, 2);
    KFR_ACCURACY_DERIVED(sinh, hyperbolic(hyperbolic_function::sinh), -20, 20, 114, 2.5);
    KFR_ACCURACY_DERIVED(cosh, hyperbolic(hyperbolic_function::cosh), -20, 20, 8, 2.5);
    KFR_ACCURACY_DERIVED(tanh, hyperbolic(hyperbolic_function::tanh), -10, 10, 45, 1);
    KFR_ACCURACY_DERIVED(coth, hyperbolic(hyperbolic_function::coth), 0.01, 10, 85, 1);
#undef KFR_ACCURACY_DERIVED

    binary(
        out, ctx, "atan2", [](auto y, auto x) { return kfr::atan2(y, x); },
        [](bf_t* r, const bf_t* y, const bf_t* x) { return bf_atan2(r, y, x, precision, nearest); },
        { -10, 10 }, { -10, 10 }, 3, 1);
    binary(
        out, ctx, "pow", [](auto x, auto y) { return kfr::pow(x, y); },
        [](bf_t* r, const bf_t* x, const bf_t* y) { return bf_pow(r, x, y, precision, nearest); },
        { 0.125, 10 }, { -8, 8 }, 48, 8);

    const auto remainder = [](bf_t* r, const bf_t* x, const bf_t* y)
    { return bf_rem(r, x, y, precision, nearest, BF_RNDZ); };
    binary(
        out, ctx, "fmod", [](auto x, auto y) { return kfr::fmod(x, y); }, remainder, { -100, 100 },
        { 0.5, 10 }, 0, 0);
    binary(
        out, ctx, "rem",
        [](auto x, auto y)
        {
            if constexpr (std::is_floating_point_v<decltype(x)>)
                return kfr::rem(vec<decltype(x), 1>(x), vec<decltype(y), 1>(y))[0];
            else
                return kfr::rem(x, y);
        },
        remainder, { -100, 100 }, { 0.5, 10 }, 0, 0);
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        const char* report_path = argc > 1 ? argv[1] : "math-accuracy.json";
        std::ofstream file(report_path);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.imbue(std::locale::classic());
        file << std::setprecision(std::numeric_limits<double>::max_digits10);
        file << "{\n  \"schema_version\": 1,\n  \"results\": [\n";
        context ctx;
        report output{ file };
        run(output, &ctx.value);
        file << "\n  ]\n}\n";
        file.close();
        std::cout << "Math accuracy report written to: " << report_path << '\n';
        return output.passed ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "math accuracy: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
