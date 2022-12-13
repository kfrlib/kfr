# Upgrade to KFR 5

The KFR 5 release intoduced several changes to architecture, naming and semantics so any code using KFR needs to be adjusted.

Here is the list of changes.

### CMake variables now have `KFR_` prefix

Examples of changes:
```cmake
ENABLE_DFT               -> KFR_ENABLE_DFT
ENABLE_DFT_NP            -> KFR_ENABLE_DFT_NP
ENABLE_DFT_MULTIARCH     -> KFR_ENABLE_DFT_MULTIARCH
ENABLE_CAPI_BUILD        -> KFR_ENABLE_CAPI_BUILD
DISABLE_CLANG_EXTENSIONS -> KFR_DISABLE_CLANG_EXTENSIONS
REGENERATE_TESTS         -> KFR_REGENERATE_TESTS
CPU_ARCH                 -> KFR_ARCH
```

The following variables are unchanged.
```
ENABLE_TESTS
ENABLE_EXAMPLES
KFR_EXTENDED_TESTS
KFR_STD_COMPLEX
```


### Argument sizes must be equal

`expression_function` arguments must have the same or compatible sizes (shapes).

Previously, KFR allowed arguments with different sizes, effectively taking the minimum of
the sizes. From KFR 5 the sizes must be equal, or, alternatively, size of some arguments may be 1, values will be broadcasted to match the sizes of other arguments.


The operations on different arguments will result in the following shapes:
```
`x` denotes any operator or function.
`inf` is `kfr::infinite_size`
{} x {} = {}                  # two scalars produce scalar too
{} x {2} = {2}                # scalar broadcasted to 1-dim
{2,2} x {} = {2,2}            # scalar broadcasted to 2-dims
{1} x {2} = {2}               # broadcasted because size=1
{2} x {3} = error in KFR 5, {2} in KFR 4
{1,5} x {5,1} = {5,5}         # row x column -> matrix
{5} x {5,1} = {5,5}           # same with auto broadcasting
{inf} x {2} = {2}             # infinite arguments are allowed everywhere
{inf,inf} x {5,1} = {5,1}     # infinite arguments are allowed everywhere
{2,2} x {inf} = {2,2}         # infinite arguments are allowed everywhere
{inf,inf} x {inf} = {inf,inf} # infinite arguments are allowed everywhere
```

The resulting shape can be tested with `internal_generic::common_shape` function.

### Scalars are 0-dimensions values

Scalars are now 0-dimensions values, use `dimensions<1>(scalar)` to change it to 1-dim infinite sequences as it was in KFR 4.

In expressions, scalars are automatically broadcasted to higher dimensions as needed.

### `std` aliases have been removed from `cometa` namespace

Original std classes and variables should be used instead. `_t` (for types) or `_v` (values) suffix may be needed.

Examples of code changes:
```c++
cometa::invoke_result  -> std::invoke_result_t
cometa::enable_if      -> std::enable_if_t
cometa::common_type    -> std::common_type_t
cometa::is_same        -> std::is_same_v
cometa::is_invocable_r -> std::is_invocable_r_v
```

### All expressions and some classes have been moved out of `internal` namespace

Examples of code changes:
```
kfr::internal::biquad_state        -> kfr::biquad_state
kfr::internal::expression_slice    -> kfr::expression_slice
kfr::internal::expression_function -> kfr::expression_function
```

### Expressions 'pointers' have been renamed to 'handles'.

```
expression_pointer -> expression_handle
to_pointer -> to_handle
```

Also, handles now require their number of dimensions to be specified.

### `expression_reduce` new template argument

`expression_reduce` changed its prototype and now have Dims as its second template parameter.
