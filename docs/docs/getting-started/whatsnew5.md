# What's new in KFR 5

* New `tensor<T, dims>` class for multidimensional data (like nparray)
* All builtin expressions support multiple dimensions
* Exception support (may be configured to call user-supplied function or std::abort)
* [changes required] CMake variables now have `KFR_` prefix
* Template parameter deduction for `vec`, so `vec{1, 2}` is the same as `vec<int, 2>{1, 2}`
* [changes required] `random_state` is now architecture-agnostic and defined in `kfr` namespace
* All expression classes have been moved from `kfr::CMT_ARCH_NAME::internal` to `kfr::CMT_ARCH_NAME` namespace
* `expression_traits<T>` introduced to support interpreting any object as kfr expression
* [changes required] User-defined expressions should be rewritten to be used in KFR5
* Out-of-class assign operators for all input & output expressions
* `round.hpp`, `clamp.hpp`, `select.hpp`, `sort.hpp`, `saturation.hpp`, `min_max.hpp`, `logical.hpp`, `abs.hpp` headers have been moved to `simd` module
* `state_holder.hpp` has been moved to `base` module
* All code related to expressions have been moved to `base` module
* `vec<T, N>::front()` and `vec<T, N>::front()` are now writable
* `set_elements` functions for output expressions like `get_elements` for input expressions
