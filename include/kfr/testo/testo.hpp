#pragma once

#include "comparison.hpp"

#include <algorithm>
#include <ctime>
#include <functional>
#include <sstream>
#include <utility>
#include <vector>
#ifdef TESTO_MPFR
#include <mpfr/mpfr.hpp>
#include <mpfr/mpfr_tostring.hpp>
#endif
#include "../ext/console_colors.hpp"
#include <chrono>
#include <cmath>

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wexit-time-destructors")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpadded")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")

namespace testo
{

using namespace cometa;

#ifdef TESTO_MPFR
using reference_number = mpfr::number;
#else
using reference_number = long double;
#endif

#ifdef TESTO_MPFR
template <typename T>
inline double ulp_distance(const mpfr::number& reference, T test)
{
    if (std::isnan(test) && reference.isnan())
        return 0.0;
    if (std::isinf(test) && (reference.isinfinity() || mpfr::abs(reference) > std::numeric_limits<T>::max()))
    {
        if ((reference < 0 && test < 0) || (reference > 0 && test > 0))
            return 0.0;
        else
            return std::numeric_limits<double>::infinity();
    }
    mpfr::number testreal = test;
    T next                = std::nexttoward(test, std::numeric_limits<long double>::infinity());
    mpfr::number ulp      = testreal - mpfr::number(next);
    return std::abs(static_cast<double>((reference - testreal) / ulp));
}
inline std::string number_to_string(const mpfr::number& reference, int precision)
{
    return mpfr::to_string(reference, precision, 'g');
}
#else
template <typename T>
inline double ulp_distance(long double reference, T test)
{
    if (__builtin_isnan(test) && __builtin_isnan(reference))
        return 0.0;
    if (__builtin_isinf(test) &&
        (__builtin_isinf(reference) || std::fabs(reference) > std::numeric_limits<T>::max()))
    {
        if ((reference < 0 && test < 0) || (reference > 0 && test > 0))
            return 0.0;
        else
            return std::numeric_limits<double>::infinity();
    }
    long double test80 = test;
    T next             = std::nexttoward(test, std::numeric_limits<long double>::infinity());
    long double ulp    = test80 - static_cast<long double>(next);
    return std::abs(static_cast<double>((reference - test80) / ulp));
}
#endif

using namespace console_colors;

inline std::vector<std::string> split(const std::string& text, char delimeter)
{
    std::string r    = text;
    size_t prev_pos  = 0;
    size_t start_pos = 0;
    std::vector<std::string> list;
    while ((start_pos = r.find(delimeter, prev_pos)) != std::string::npos)
    {
        list.push_back(text.substr(prev_pos, start_pos - prev_pos));
        prev_pos = start_pos + 1;
    }
    list.push_back(text.substr(prev_pos));
    return list;
}

struct test_case;

inline test_case*& active_test()
{
    static test_case* instance = nullptr;
    return instance;
}

struct test_case
{
    using test_func = void (*)();

    static std::vector<test_case*>& tests()
    {
        static std::vector<test_case*> list;
        return list;
    }

    test_case(test_func func, const char* name)
        : func(func), name(name), success(0), failed(0), time(0), show_progress(false)
    {
        tests().push_back(this);
    }

    bool run(bool show_successful)
    {
        using namespace std::chrono;
        using time_point = high_resolution_clock::time_point;
        {
            console_color cc(Cyan);
            printfmt("[{}]", padcenter(11, std::string("RUN"), '-'));
        }
        printfmt(" {}...\n", name);
        time_point start = high_resolution_clock::now();
        active_test()    = this;
        func();
        active_test()   = nullptr;
        time_point stop = high_resolution_clock::now();
        time            = duration_cast<duration<double>>(stop - start).count();

        {
            console_color cc(failed ? Red : Green);
            printfmt("[{}] {} subtests of {}\n", padcenter(11, failed ? "ERROR" : "SUCCESS", '-'),
                     failed ? failed : success, success + failed);
        }
        if (failed)
        {
            for (const subtest& s : subtests)
            {
                if ((s.success && show_successful) || !s.success)
                {
                    if (!s.comment.empty())
                        printfmt("    {}:\n", s.comment);
                    {
                        console_color cc(s.success ? Green : Red);
                        printfmt("    {} ", s.success ? "[success]" : "[fail]   ");
                    }
                    printfmt("{}\n", s.text);
                }
            }
            console_color cc(White);
        }
        return !failed;
    }

    void check(bool result, const std::string& value, const char* expr)
    {
        subtests.push_back(subtest{ result, as_string(padleft(22, expr), " | ", value), comment });
        result ? success++ : failed++;
        if (show_progress)
        {
            if (result)
            {
                console_color cc(Green);
                print(".");
            }
            else
            {
                console_color cc(Red);
                print("E");
            }
        }
    }

    template <typename Op, typename L, typename R>
    void check(const comparison<Op, L, R>& comparison, const char* expr)
    {
        bool result = comparison();
        check(result, as_string(comparison.left, " ", Op::op(), " ", comparison.right), expr);
    }

    template <typename L>
    void check(const half_comparison<L>& comparison, const char* expr)
    {
        bool result = comparison.left ? true : false;
        check(result, as_string(comparison.left), expr);
    }

    void set_comment(const std::string& text)
    {
        comment = text;
        if (show_progress)
        {
            println();
            println(comment, ":");
        }
    }

    struct subtest
    {
        bool success;
        std::string text;
        std::string comment;
    };

    test_func func;
    const char* name;
    std::vector<subtest> subtests;
    std::string comment;
    int success;
    int failed;
    double time;
    bool show_progress;
};

template <typename Number>
struct statistics
{
    Number minimum;
    Number maximum;
    double sum;
    unsigned long long count;
    std::vector<Number> values;
    void reset() { *this = statistics<Number>(); }
    std::string str()
    {
        return format("{} ... {} (avg={}, median={})\n", minimum, maximum, cometa::fmt<'f', 2>(average()),
                      median());
    }
    double average() const { return sum / count; }
    Number median()
    {
        std::sort(values.begin(), values.end());
        return values.empty() ? Number() : values[values.size() / 2];
    }
    statistics()
        : sum(), count(), minimum(std::numeric_limits<Number>::max()),
          maximum(std::numeric_limits<Number>::min())
    {
    }
    void operator()(Number x)
    {
        minimum = std::min(minimum, x);
        maximum = std::max(maximum, x);
        sum += x;
        count++;
        values.push_back(x);
    }
};

template <typename Arg0, typename Fn>
void matrix(named_arg<Arg0>&& arg0, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), [&](auto v0) {
        active_test()->set_comment(as_string(arg0.name, " = ", v0));
        fn(v0);
    });
    if (active_test()->show_progress)
        println();
}

template <typename Arg0, typename Arg1, typename Fn>
void matrix(named_arg<Arg0>&& arg0, named_arg<Arg1>&& arg1, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), std::forward<Arg1>(arg1.value), [&](auto v0, auto v1) {
        active_test()->set_comment(as_string(arg0.name, " = ", v0, ", ", arg1.name, " = ", v1));
        fn(v0, v1);
    });
    if (active_test()->show_progress)
        println();
}

template <typename Arg0, typename Arg1, typename Arg2, typename Fn>
void matrix(named_arg<Arg0>&& arg0, named_arg<Arg1>&& arg1, named_arg<Arg2>&& arg2, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), std::forward<Arg1>(arg1.value), std::forward<Arg2>(arg2.value),
             [&](auto v0, auto v1, auto v2) {
                 active_test()->set_comment(
                     as_string(arg0.name, " = ", v0, ", ", arg1.name, " = ", v1, ", ", arg2.name, " = ", v2));
                 fn(v0, v1, v2);
             });
    if (active_test()->show_progress)
        println();
}

CMT_UNUSED static int run_all(const std::string& name = std::string(), bool show_successful = false)
{
    std::vector<test_case*> success;
    std::vector<test_case*> failed;
    for (test_case* t : test_case::tests())
    {
        if (name.empty() || t->name == name)
            t->run(show_successful) ? success.push_back(t) : failed.push_back(t);
    }
    printfmt("{}\n", std::string(79, '='));
    if (!success.empty())
    {
        console_color cc(Green);
        printfmt("[{}]", padcenter(11, "SUCCESS", '-'));
        printfmt(" {} tests\n", success.size());
    }
    if (!failed.empty())
    {
        console_color cc(Red);
        printfmt("[{}]", padcenter(11, "ERROR", '-'));
        printfmt(" {} tests\n", failed.size());
    }
    return static_cast<int>(failed.size());
}

template <typename T1, typename T2>
void assert_is_same()
{
    static_assert(std::is_same<T1, T2>::value, "");
}
template <typename T1, typename T2>
void assert_is_same_decay()
{
    static_assert(std::is_same<cometa::decay<T1>, cometa::decay<T2>>::value, "");
}

#define TESTO_CHECK(...)                                                                                     \
    do                                                                                                       \
    {                                                                                                        \
        ::testo::active_test()->check(::testo::make_comparison() <= __VA_ARGS__, #__VA_ARGS__);              \
    } while (0)

#define TESTO_TEST(name)                                                                                     \
    static void test_function_##name();                                                                      \
    ::testo::test_case test_case_##name(&test_function_##name, #name);                                       \
    static void CMT_NOINLINE test_function_##name()

#define TESTO_DTEST(name)                                                                                    \
    template <typename>                                                                                      \
    static void disabled_test_function_##name()

#ifndef TESTO_NO_SHORT_MACROS
#define CHECK TESTO_CHECK
#define TEST TESTO_TEST
#define DTEST TESTO_DTEST
#endif
}

CMT_PRAGMA_GNU(GCC diagnostic pop)
