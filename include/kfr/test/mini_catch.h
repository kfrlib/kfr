/*
 * mini_catch.h — a tiny single-header "Catch2-ish" test framework for C.
 *
 * Features:
 *   - TEST_CASE("some name") -> automatic registration, name is a string
 *   - CHECK(expr)       -> records failure if expr is false, test keeps running
 *   - CHECK_FALSE(expr) -> records failure if expr is true,  test keeps running
 *   - REQUIRE(expr)     -> records failure and aborts the current test immediately
 *   - Failures print which test they happened in, plus file:line
 *
 * Compiler support:
 *   Registration relies on __attribute__((constructor)), a GCC/Clang
 *   extension. It works on GCC, Clang, and clang-cl (clang running in
 *   MSVC-compatible driver mode).
 *
 */
#ifndef MINI_CATCH_H
#define MINI_CATCH_H

#ifdef __cplusplus
#error "mini_catch.h is a C-only test framework and must not be compiled as C++."
#endif

#if !defined(__GNUC__) && !defined(__clang__)
#error                                                                                                       \
    "mini_catch.h needs GCC, Clang, or clang-cl (for __attribute__((constructor))); plain MSVC cl.exe is not supported."
#endif

#include <stdio.h>
#include <setjmp.h>

typedef struct TestCase
{
    const char* name;
    void (*func)(void);
    struct TestCase* next;
} TestCase;

extern TestCase* mc_tests_head;
extern int mc_current_failures;
extern const char* mc_current_test_name;
extern jmp_buf mc_require_jump;

/* Two-level indirection so __LINE__ is expanded to a number *before*
 * being pasted into an identifier, instead of pasting the literal
 * text "__LINE__". */
#define MC_CONCAT_(a, b) a##b
#define MC_CONCAT(a, b) MC_CONCAT_(a, b)
#define MC_UNIQUE(prefix) MC_CONCAT(prefix, __LINE__)

#define TEST_CASE(test_name_str)                                                                             \
    static void MC_UNIQUE(mc_test_fn_)(void);                                                                \
    static TestCase MC_UNIQUE(mc_test_case_) = { (test_name_str), MC_UNIQUE(mc_test_fn_), NULL };            \
    __attribute__((constructor)) static void MC_UNIQUE(mc_test_register_)(void)                              \
    {                                                                                                        \
        MC_UNIQUE(mc_test_case_).next = mc_tests_head;                                                       \
        mc_tests_head                 = &MC_UNIQUE(mc_test_case_);                                           \
    }                                                                                                        \
    static void MC_UNIQUE(mc_test_fn_)(void)

#define CHECK(...)                                                                                           \
    do                                                                                                       \
    {                                                                                                        \
        if (!(__VA_ARGS__))                                                                                  \
        {                                                                                                    \
            printf("    [%s] CHECK failed: %s  (%s:%d)\n", mc_current_test_name, #__VA_ARGS__, __FILE__,     \
                   __LINE__);                                                                                \
            mc_current_failures++;                                                                           \
        }                                                                                                    \
    } while (0)

#define CHECK_FALSE(...)                                                                                     \
    do                                                                                                       \
    {                                                                                                        \
        if ((__VA_ARGS__))                                                                                   \
        {                                                                                                    \
            printf("    [%s] CHECK_FALSE failed: %s  (%s:%d)\n", mc_current_test_name, #__VA_ARGS__,         \
                   __FILE__, __LINE__);                                                                      \
            mc_current_failures++;                                                                           \
        }                                                                                                    \
    } while (0)

#define REQUIRE(...)                                                                                         \
    do                                                                                                       \
    {                                                                                                        \
        if (!(__VA_ARGS__))                                                                                  \
        {                                                                                                    \
            printf("    [%s] REQUIRE failed: %s  (%s:%d)\n", mc_current_test_name, #__VA_ARGS__, __FILE__,   \
                   __LINE__);                                                                                \
            mc_current_failures++;                                                                           \
            longjmp(mc_require_jump, 1);                                                                     \
        }                                                                                                    \
    } while (0)

#endif /* MINI_CATCH_H */
