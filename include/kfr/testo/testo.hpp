/** @addtogroup testo
 *  @{
 */
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
#include "console_colors.hpp"
#include <cassert>
#include <chrono>
#include <cmath>

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wexit-time-destructors")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpadded")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wparentheses")

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
#if defined CMT_COMPILER_MSVC && !defined CMT_COMPILER_CLANG
#define TESTO__ISNAN(x) std::isnan(x)
#define TESTO__ISINF(x) std::isinf(x)
#else
#define TESTO__ISNAN(x) __builtin_isnan(x)
#define TESTO__ISINF(x) __builtin_isinf(x)
#endif

    if (TESTO__ISNAN(test) && TESTO__ISNAN(reference))
        return 0.0;
    if (TESTO__ISINF(test) &&
        (TESTO__ISINF(reference) || std::fabs(reference) > std::numeric_limits<T>::max()))
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
#undef TESTO__ISNAN
#undef TESTO__ISINF
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

struct scope
{
    std::string text;
    test_case* current_test;
    scope* parent;
    scope(std::string text);
    ~scope();
};

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
            std::fflush(stdout);
            std::fflush(stderr);
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
                    printfmt("{}({}) ", s.file, s.line);
                    printfmt("{}\n", s.text);
                }
            }
            console_color cc(White);
        }
        subtests.clear();
        return !failed;
    }

    void check(bool result, const std::string& value, const char* expr, const char* file, int line)
    {
        subtests.push_back(
            subtest{ result, as_string(padleft(22, expr), " | ", value), current_scope_text(), file, line });
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
    void check(const comparison<Op, L, R>& comparison, const char* expr, const char* file, int line)
    {
        bool result = comparison();
        check(result, as_string(comparison.left, " ", Op::op(), " ", comparison.right), expr, file, line);
    }

    template <typename L>
    void check(const half_comparison<L>& comparison, const char* expr, const char* file, int line)
    {
        bool result = comparison.left ? true : false;
        check(result, as_string(comparison.left), expr, file, line);
    }

    struct subtest
    {
        bool success;
        std::string text;
        std::string comment;
        std::string file;
        int line = 0;
    };

    void scope_changed()
    {
        if (show_progress)
        {
            println();
            println(current_scope_text(), ":");
        }
    }
    std::string current_scope_text() const
    {
        scope* s = this->current_scope;
        std::string result;
        while (s)
        {
            if (!result.empty())
                result = "; " + result;
            result = s->text + result;
            s      = s->parent;
        }
        return result;
    }

    test_func func;
    const char* name;
    std::vector<subtest> subtests;
    int success;
    int failed;
    double time;
    bool show_progress;
    scope* current_scope = nullptr;
};

inline scope::scope(std::string text)
    : text(std::move(text)), current_test(active_test()), parent(current_test->current_scope)
{
    current_test->current_scope = this;
    current_test->scope_changed();
}

inline scope::~scope()
{
    assert(active_test() == current_test);
    assert(current_test->current_scope == this);
    current_test->current_scope = parent;
}

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
    cforeach(std::forward<Arg0>(arg0.value),
             [&](auto v0)
             {
                 scope s(as_string(arg0.name, " = ", v0));
                 fn(v0);
             });
    if (active_test() && active_test()->show_progress)
        println();
}

template <typename Arg0, typename Arg1, typename Fn>
void matrix(named_arg<Arg0>&& arg0, named_arg<Arg1>&& arg1, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), std::forward<Arg1>(arg1.value),
             [&](auto v0, auto v1)
             {
                 scope s(as_string(arg0.name, " = ", v0, ", ", arg1.name, " = ", v1));
                 fn(v0, v1);
             });
    if (active_test()->show_progress)
        println();
}

template <typename Arg0, typename Arg1, typename Arg2, typename Fn>
void matrix(named_arg<Arg0>&& arg0, named_arg<Arg1>&& arg1, named_arg<Arg2>&& arg2, Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), std::forward<Arg1>(arg1.value), std::forward<Arg2>(arg2.value),
             [&](auto v0, auto v1, auto v2)
             {
                 scope s(
                     as_string(arg0.name, " = ", v0, ", ", arg1.name, " = ", v1, ", ", arg2.name, " = ", v2));
                 fn(v0, v1, v2);
             });
    if (active_test()->show_progress)
        println();
}

template <typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Fn>
void matrix(named_arg<Arg0>&& arg0, named_arg<Arg1>&& arg1, named_arg<Arg2>&& arg2, named_arg<Arg3>&& arg3,
            Fn&& fn)
{
    cforeach(std::forward<Arg0>(arg0.value), std::forward<Arg1>(arg1.value), std::forward<Arg2>(arg2.value),
             std::forward<Arg3>(arg3.value),
             [&](auto v0, auto v1, auto v2, auto v3)
             {
                 scope s(as_string(arg0.name, " = ", v0, ", ", arg1.name, " = ", v1, ", ", arg2.name, " = ",
                                   v2, arg3.name, " = ", v3));
                 fn(v0, v1, v2, v3);
             });
    if (active_test()->show_progress)
        println();
}

CMT_UNUSED static int run_all(const std::string& name = std::string(), bool show_successful = false)
{
    console_color c(White);
    std::vector<test_case*> success;
    std::vector<test_case*> failed;
    int success_checks = 0;
    int failed_checks  = 0;
    for (test_case* t : test_case::tests())
    {
        if (name.empty() || t->name == name)
        {
            t->run(show_successful) ? success.push_back(t) : failed.push_back(t);
            success_checks += t->success;
            failed_checks += t->failed;
        }
    }
    printfmt("{}\n", std::string(79, '='));
    if (!success.empty())
    {
        console_color cc(Green);
        printfmt("[{}]", padcenter(11, "SUCCESS", '-'));
        printfmt(" {}/{} tests {}/{} checks\n", success.size(), success.size() + failed.size(),
                 success_checks, success_checks + failed_checks);
    }
    if (!failed.empty())
    {
        console_color cc(Red);
        printfmt("[{}]", padcenter(11, "ERROR", '-'));
        printfmt(" {}/{} tests {}/{} checks\n", failed.size(), success.size() + failed.size(), failed_checks,
                 success_checks + failed_checks);
        for (test_case* t : failed)
        {
            print("              ", t->name, "\n");
        }
    }
    return static_cast<int>(failed.size());
}

template <typename T1, typename T2>
void assert_is_same()
{
    static_assert(std::is_same_v<T1, T2>, "");
}
template <typename T1, typename T2>
void assert_is_same_decay()
{
    static_assert(std::is_same_v<std::decay_t<T1>, std::decay_t<T2>>, "");
}

template <typename T, size_t NArgs>
struct test_data_entry
{
    T arguments[NArgs];
    T result;
};

#define TESTO_CHECK(...)                                                                                     \
    do                                                                                                       \
    {                                                                                                        \
        ::testo::active_test()->check(::testo::make_comparison() <= __VA_ARGS__, #__VA_ARGS__, __FILE__,     \
                                      __LINE__);                                                             \
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

} // namespace testo

CMT_PRAGMA_GNU(GCC diagnostic pop)
