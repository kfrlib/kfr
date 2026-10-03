# Errors, Assertions, and Runtime Checks

KFR separates failures that can be reported to a caller from development-time
assertions. Use the error-reporting facilities to reject invalid public input
or impossible runtime conditions. Use assertions to document internal
assumptions while developing and testing. They have different configuration
switches and different failure behavior.

The error types and checking macros are available from `<kfr/except.hpp>`.
Assertions and [[`safe_cast`:nosig]] are available from `<kfr/test/assert.hpp>`;
the higher-level KFR headers normally include the support they need.

## Error Categories

[[`exception`:nosig]] is KFR's base exception class and derives from
`std::exception`. Its [[`exception::what()`:nosig]] message is assembled from
the supplied arguments. [[`logic_error`:nosig]] and [[`runtime_error`:nosig]]
derive from it and distinguish the reason for failure:

- Use [[`logic_error`:nosig]] for an API contract or library-state violation:
  a caller supplied incompatible dimensions, attempted an operation before
  initialization, or requested an unsupported configuration.
- Use [[`runtime_error`:nosig]] for a failure that depends on runtime
  circumstances, such as unavailable input or a resource that cannot be used.

[[`KFR_REPORT_LOGIC_ERROR`]] and [[`KFR_REPORT_RUNTIME_ERROR`]] report an error
unconditionally. The `KFR_RUNTIME_CHECK` and `KFR_LOGIC_CHECK` macros add the
usual condition around those categories.

```c++
#include <kfr/except.hpp>
|||#include <kfr/base.hpp>

using namespace kfr;

void copy_samples(float* output, const float* input, size_t count)
{
    KFR_RUNTIME_CHECK(output != nullptr, "output must not be null");
    KFR_RUNTIME_CHECK(input != nullptr, "input must not be null");

    for (size_t i = 0; i < count; ++i)
        output[i] = input[i];
}

void set_channel_count(size_t channels)
{
    KFR_LOGIC_CHECK(channels > 0 && channels <= 8,
                    "unsupported channel count: ", channels);
}
```

```c++
|||TEST_CASE("runtime_checks.md/copy_samples")
|||{
float input[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
float output[4] = {};
copy_samples(output, input, 4);
|||CHECK(output[0] == 1.0f);
|||CHECK(output[3] == 4.0f);
|||}
```

The message arguments are forwarded to KFR's formatting support, so a check
can combine text and values without manually building a `std::string`.

## Exceptions-Enabled and Exceptions-Free Builds

When [[`KFR_HAS_EXCEPTIONS`]] is true, a reporting macro throws the matching
KFR exception. Code at a library boundary can catch the base type or the more
specific category:

```c++
|||TEST_CASE("runtime_checks.md/logic_error catch")
|||{
try
{
    set_channel_count(0);
}
catch (const logic_error& error)
{
    // error.what() describes the violated contract.
}
catch (const exception& error)
{
    // Handles other KFR error categories.
}
|||}
```

When exceptions are unavailable, the same reporting macros write a diagnostic
to standard error and call `std::abort()`. There is no recoverable error return
hidden behind the macros. If a program must recover in an exceptions-free
build, validate the input itself and use an explicit status or result type in
its own API.

> [!note]
> [[`KFR_REPORT_LOGIC_ERROR`]] and [[`KFR_REPORT_RUNTIME_ERROR`]] remain active
> in every build. Use them for failures that must never be ignored.

## Enabling and Disabling Checks

Define `KFR_DISABLE_CHECKS` to remove [[`KFR_RUNTIME_CHECK`]] and
[[`KFR_LOGIC_CHECK`]]. In that configuration they expand to a no-op: neither
the condition nor the message expressions are evaluated.

```c++
size_t checked_divide(size_t value, size_t divisor)
{
    KFR_RUNTIME_CHECK(divisor != 0, "division by zero");
    return value / divisor;
}
```

Do not depend on the check for necessary program logic. With checks disabled,
the example still performs the division, and `divisor == 0` remains undefined
behavior. Write a normal branch when the program must handle the case:

```c++
bool try_divide(size_t value, size_t divisor, size_t& result)
{
    if (divisor == 0)
        return false;

    result = value / divisor;
    return true;
}
```

A useful rule is to put externally observable recovery in ordinary control
flow, and use a KFR check to state the contract that should make the failure
unreachable.

```c++
|||TEST_CASE("runtime_checks.md/checked_divide and try_divide")
|||{
|||CHECK(checked_divide(10, 2) == 5);
size_t result = 0;
bool ok = try_divide(10, 2, result);
|||CHECK(ok);
|||CHECK(result == 5);
bool ok_zero = try_divide(10, 0, result);
|||CHECK_FALSE(ok_zero);
|||}
```

## Development Assertions

[[`KFR_ASSERT`]] verifies an internal assumption. In an active build, an
assertion failure prints the expression, source location, and—when the
expression is a comparison—the observed operands. It then invokes
[[`KFR_BREAKPOINT`]], which stops in a debugger or traps the process.
[[`ASSERT`]] is a short alias unless `KFR_NO_SHORT_MACROS` is defined.

```c++
#include <kfr/test/assert.hpp>

using namespace kfr;

float sample_at(const float* data, size_t size, size_t index)
{
    KFR_ASSERT(data != nullptr);
    KFR_ASSERT(index < size);
    return data[index];
}

|||TEST_CASE("runtime_checks.md/sample_at")
|||{
|||float data[3] = { 10.0f, 20.0f, 30.0f };
|||CHECK(sample_at(data, 3, 1) == 20.0f);
|||}
```

Assertions are active by default. They are disabled when either `NDEBUG` or
`KFR_ASSERTION_OFF` is defined, and can be forced on with `KFR_ASSERTION_ON`.
An inactive assertion still requires syntactically valid C++, but it does not
evaluate its expression. Consequently, assertions must not perform required
side effects:

```c++
|||void assert_example(size_t index, size_t count)
|||{
KFR_ASSERT(++index < count); // Incorrect: ++index disappears in inactive builds.
|||}
```

Keep an assertion to one predicate or comparison, such as `index < size`.
This produces the clearest failure report and avoids concealing a compound
condition in a development-only diagnostic.

Applications that need to direct assertion output to their own test harness or
logger can define `KFR_CUSTOM_ASSERTION_PRINT` and provide:

```c++
||||||||||
namespace kfr
{
void assertion_failed(const std::string& description, const char* file, int line);
}
||||||||||
```

The replacement should record or display the failure; the assertion macro
still performs its breakpoint/trap afterward.

## Checked Integral Conversion

[[`safe_cast`:nosig]] converts between integral types after asserting that the
source fits in the destination type. It accounts for signed-to-unsigned and
unsigned-to-signed conversions before applying `static_cast`.

```c++
#include <kfr/test/assert.hpp>
#include <kfr/base.hpp>

using namespace kfr;

|||TEST_CASE("runtime_checks.md/safe_cast")
|||{
int packet_length = 1024;
u16 stored_length = safe_cast<u16>(packet_length);

// In an active-assertion build, this reports a failed range check.
int negative = -1;
// u16 invalid = safe_cast<u16>(negative);
|||CHECK(stored_length == 1024);
|||}
```

[[`safe_cast`:nosig]] accepts integral source and destination types only. It is
a development guard, not a saturating conversion and not an exception-based
validation API. When assertions are inactive, it is an unchecked `static_cast`.
Validate untrusted values explicitly before calling it if a truncated or wrapped
result would be unsafe.

## Choosing the Right Facility

| Situation | Recommended facility |
|---|---|
| A public KFR-style operation receives an invalid argument | [[`KFR_LOGIC_CHECK`]] |
| A runtime failure must stop processing and be reported | [[`KFR_RUNTIME_CHECK`]] or [[`KFR_REPORT_RUNTIME_ERROR`]] |
| A local invariant should be caught while developing | [[`KFR_ASSERT`]] |
| A conversion should be range-checked in debug/testing builds | [[`safe_cast`:nosig]] |
| Production code must recover from bad input | Explicit validation and a status/result path |

Checks describe contracts; assertions describe assumptions. Keeping that split
makes it clear which conditions callers can diagnose and which ones indicate a
programming mistake.

## See Also

- [Numeric, Compound, and Complex Types](types.md)
- [Memory, Alignment, and Ownership](memory.md)
- [KFR Knowledge Base](../../advanced/kb.md)
