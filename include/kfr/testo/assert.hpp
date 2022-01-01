/** @addtogroup testo
 *  @{
 */
#pragma once

#include "comparison.hpp"

#if defined(CMT_COMPILER_MSVC)
#include <intrin.h>
#define TESTO_BREAKPOINT __debugbreak()
#else
#if defined(__i386__) || defined(__x86_64__)
#define TESTO_BREAKPOINT __asm__ __volatile__("int $0x03")
#else
#define TESTO_BREAKPOINT __builtin_trap()
#endif
#endif

namespace testo
{

#ifdef TESTO_CUSTOM_ASSERTION_PRINT
void assertion_failed(const std::string& string, const char* file, int line);
#else
inline void assertion_failed(const std::string& string, const char* file, int line)
{
    errorln("Assertion failed at ", file, ":", line);
    errorln(string);
    errorln();
    std::fflush(stderr);
}
#endif

bool check_assertion(...);

template <typename Op, typename L, typename R>
bool check_assertion(const comparison<Op, L, R>& comparison, const char* expr, const char* file, int line)
{
    bool result = comparison();
    if (!result)
    {
        assertion_failed(
            as_string(padleft(22, expr), " | ", comparison.left, " ", Op::op(), " ", comparison.right), file,
            line);
    }
    return result;
}

template <typename L>
bool check_assertion(const half_comparison<L>& comparison, const char* expr, const char* file, int line)
{
    bool result = static_cast<bool>(comparison.left);
    if (!result)
    {
        assertion_failed(as_string(padleft(22, expr), " | ", comparison.left), file, line);
    }
    return result;
}

#define TESTO_ASSERT_ACTIVE(...)                                                                             \
    do                                                                                                       \
    {                                                                                                        \
        if (!::testo::check_assertion(::testo::make_comparison() <= __VA_ARGS__, #__VA_ARGS__, __FILE__,     \
                                      __LINE__))                                                             \
            TESTO_BREAKPOINT;                                                                                \
    } while (0)

#define TESTO_ASSERT_INACTIVE(...)                                                                           \
    do                                                                                                       \
    {                                                                                                        \
    } while (false && (__VA_ARGS__))

#if defined(TESTO_ASSERTION_ON) || !(defined(NDEBUG) || defined(TESTO_ASSERTION_OFF))

#define TESTO_ASSERT TESTO_ASSERT_ACTIVE

#else

#define TESTO_ASSERT TESTO_ASSERT_INACTIVE

#endif

#ifndef TESTO_NO_SHORT_MACROS
#define ASSERT TESTO_ASSERT
#endif

} // namespace testo
