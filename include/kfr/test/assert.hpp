#pragma once

#include "comparison.hpp"

#if defined(KFR_COMPILER_MSVC)
#include <intrin.h>
#define KFR_BREAKPOINT __debugbreak()
#else
#if defined(__i386__) || defined(__x86_64__)
#define KFR_BREAKPOINT __asm__ __volatile__("int $0x03")
#else
#define KFR_BREAKPOINT __builtin_trap()
#endif
#endif

namespace kfr
{

#ifdef KFR_CUSTOM_ASSERTION_PRINT
/**
 * @brief Reports a failed assertion to the user.
 *
 * Defined by the application when @c KFR_CUSTOM_ASSERTION_PRINT is set, so that
 * assertion failures can be routed to a custom sink (e.g. a test harness or
 * logging framework) instead of @c stderr.
 *
 * @param string Human-readable description of the failed comparison.
 * @param file   Name of the source file where the assertion was evaluated.
 * @param line   One-based line number within @p file.
 */
void assertion_failed(const std::string& string, const char* file, int line);
#else
/**
 * @brief Prints a failed assertion to @c stderr.
 *
 * Default implementation used when @c KFR_CUSTOM_ASSERTION_PRINT is not defined.
 * Emits the source location followed by the assertion description and flushes
 * @c stderr.
 *
 * @param string Human-readable description of the failed comparison.
 * @param file   Name of the source file where the assertion was evaluated.
 * @param line   One-based line number within @p file.
 */
inline void assertion_failed(const std::string& string, const char* file, int line)
{
    errorln("Assertion failed at ", file, ":", line);
    errorln(string);
    errorln();
    std::fflush(stderr);
}
#endif

/**
 * @brief SFINAE fallback for non-comparison arguments.
 *
 * Always returns @c true so that @c KFR_ASSERT can be applied to arbitrary
 * expressions without a comparison object (in which case the expression is
 * only evaluated for its side effects and never reported as failed).
 */
bool check_assertion(...);

/**
 * @brief Evaluates a full comparison and reports it on failure.
 *
 * Invokes the comparison functor, and if it returns @c false, formats the
 * operands and the operator through @ref assertion_failed.
 *
 * @param comparison The comparison object produced by @ref make_comparison.
 * @param expr       Textual representation of the asserted expression.
 * @param file       Name of the source file where the assertion was evaluated.
 * @param line       One-based line number within @p file.
 * @return @c true if the comparison holds, @c false otherwise.
 */
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

/**
 * @brief Evaluates a half comparison (a bare boolean operand) and reports it
 * on failure.
 *
 * Unlike @ref check_assertion(const comparison<Op,L,R>&,...) this overload does
 * not compare two operands; it treats the stored value itself as the boolean
 * result.
 *
 * @param comparison The half comparison holding the boolean-like operand.
 * @param expr       Textual representation of the asserted expression.
 * @param file       Name of the source file where the assertion was evaluated.
 * @param line       One-based line number within @p file.
 * @return @c true if the operand is truthy, @c false otherwise.
 */
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

/**
 * @brief Active form of @ref KFR_ASSERT.
 *
 * Builds a comparison object from the leftmost operand via @ref make_comparison,
 * evaluates it through @ref check_assertion, and triggers a debugger breakpoint
 * (see @c KFR_BREAKPOINT) when the assertion fails.
 */
#define KFR_ASSERT_ACTIVE(...)                                                                               \
    do                                                                                                       \
    {                                                                                                        \
        if (!::kfr::check_assertion(::kfr::make_comparison() <= __VA_ARGS__, #__VA_ARGS__, __FILE__,         \
                                    __LINE__))                                                               \
            KFR_BREAKPOINT;                                                                                  \
    } while (0)

/**
 * @brief Inactive form of @ref KFR_ASSERT.
 *
 * Expands to a no-op that still parses @p __VA_ARGS__ (so the asserted
 * expression remains syntactically valid) but never evaluates it.
 */
#define KFR_ASSERT_INACTIVE(...)                                                                             \
    do                                                                                                       \
    {                                                                                                        \
    } while (false && (__VA_ARGS__))

#if defined(KFR_ASSERTION_ON) || !(defined(NDEBUG) || defined(KFR_ASSERTION_OFF))

#define KFR_ASSERT KFR_ASSERT_ACTIVE

#else

#define KFR_ASSERT KFR_ASSERT_INACTIVE

#endif

#ifndef KFR_NO_SHORT_MACROS
/**
 * @brief Short alias for @ref KFR_ASSERT, available unless
 * @c KFR_NO_SHORT_MACROS is defined.
 */
#define ASSERT KFR_ASSERT
#endif

/**
 * @brief Casts between integral types after range-checking the source value.
 *
 * Performs signed/unsigned-aware bounds checks against the destination type's
 * @c std::numeric_limits range and triggers @ref KFR_ASSERT on any out-of-range
 * value. The comparisons themselves are written so that implicit conversions
 * never produce misleading results (e.g. a signed-to-unsigned check is split
 * into a sign test followed by an upper-bound test).
 *
 * @tparam OutType Destination integral type.
 * @tparam InType  Source integral type.
 * @param val Value to convert.
 * @return @p val converted to @c OutType via @c static_cast.
 */
template <typename OutType, typename InType>
inline OutType safe_cast(const InType& val)
{
    static_assert(std::is_integral<InType>::value && std::is_integral<OutType>::value,
                  "safe_cast is for numeric types only");
    if (std::is_signed<InType>::value && std::is_signed<OutType>::value) // S->S
    {
        ASSERT(val >= std::numeric_limits<OutType>::min());
        ASSERT(val <= std::numeric_limits<OutType>::max());
    }
    else if (!std::is_signed<InType>::value && !std::is_signed<OutType>::value) // U->U
    {
        ASSERT(val <= std::numeric_limits<OutType>::max());
    }
    else if (std::is_signed<InType>::value && !std::is_signed<OutType>::value) // S->U
    {
        ASSERT(val >= 0);
        ASSERT(val <= std::numeric_limits<OutType>::max());
        // val will be converted to an unsigned number for the above comparison.
        // it's safe because we've already checked that it is positive
    }
    else // U->S
    {
        ASSERT(val <= std::numeric_limits<OutType>::max());
        // std::numeric_limits<OutType>::max() will be converted to an unsigned number for the above
        // comparison. it's also safe here
    }
    return static_cast<OutType>(val);
}

} // namespace kfr
