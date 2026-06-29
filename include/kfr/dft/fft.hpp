/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once

#include "../base/basic_expressions.hpp"
#include "../base/memory.hpp"
#include "../base/tensor.hpp"
#include "../base/univector.hpp"
#include "../math/sin_cos.hpp"
#include "../simd/complex.hpp"
#include "../simd/constants.hpp"
#include <bitset>
#include <chrono>
#include <functional>
#include <initializer_list>

KFR_PRAGMA_GNU(GCC diagnostic push)
#if KFR_HAS_WARNING("-Wshadow")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
#endif
#if KFR_HAS_WARNING("-Wundefined-inline")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wundefined-inline")
#endif

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4100))

namespace kfr
{

#define DFT_MAX_STAGES 32

/// @brief Tag type selecting the direct (forward) DFT direction.
using cdirect_t = cfalse_t;
/// @brief Tag type selecting the inverse DFT direction.
using cinvert_t = ctrue_t;

/// @brief Base structure representing a single DFT stage.
///
/// A DFT plan is composed of one or more stages executed in sequence. Each stage
/// performs a portion of the overall transform (e.g. a radix pass, a bit-reversal
/// reorder, or a repacking step). Concrete stages derive from this class and
/// implement `do_execute` for both direct and inverse directions.
template <typename T>
struct dft_stage
{
    size_t radix      = 0; ///< Radix of the stage (number of butterfly arms).
    size_t stage_size = 0; ///< Number of complex elements processed by this stage.
    size_t data_size  = 0; ///< Size in bytes of the stage's internal data (e.g. twiddles).
    size_t temp_size  = 0; ///< Size in bytes of scratch buffer required by this stage.
    u8* data          = nullptr; ///< Pointer to the stage's internal data buffer.
    size_t repeats    = 1; ///< Number of recursive repetitions used during execution.
    size_t out_offset = 0; ///< Offset (in complex elements) between successive recursive outputs.
    size_t blocks     = 0; ///< Number of independent blocks processed by the stage.
    size_t user       = 0; ///< Stage-specific user value (e.g. log2 of the stage size).
    const char* name  = nullptr; ///< Human-readable name of the stage (for `dump()`).
    bool recursion    = false; ///< Whether the stage uses recursive execution.
    bool can_inplace  = true; ///< Whether the stage can operate in-place.
    bool need_reorder = true; ///< Whether the stage output requires bit-reversal reordering.

    inline static void* operator new(size_t size) noexcept
    {
        return details::aligned_malloc(size, default_memory_alignment);
    }
    inline static void operator delete(void* ptr) noexcept { details::aligned_free(ptr); }
#ifdef __cpp_aligned_new
    inline static void* operator new(size_t size, std::align_val_t al) noexcept
    {
        return details::aligned_malloc(size, std::max(default_memory_alignment, static_cast<size_t>(al)));
    }
    inline static void operator delete(void* ptr, std::align_val_t al) noexcept
    {
        details::aligned_free(ptr);
    }
#endif

#ifdef KFR_DFT_MEASURE_STAGE_TIME
    double time = 0;
#endif

    void initialize(size_t size) { do_initialize(size); }

    /// @brief Prints the stage parameters to stdout for debugging.
    virtual void dump() const
    {
        printf("%s: %zu, %zu, %zu, %zu, %zu, %zu, %zu, %d, %d\n", name ? name : "unnamed", radix, stage_size,
               data_size, temp_size, repeats, out_offset, blocks, recursion, can_inplace);
    }
    /// @brief Copies the input into the stage's working buffer.
    /// @param invert Direction flag (true for inverse).
    /// @param out Destination buffer.
    /// @param in Source buffer.
    /// @param size Number of complex elements to copy.
    virtual void copy_input(bool invert, complex<T>* out, const complex<T>* in, size_t size)
    {
        builtin_memcpy(out, in, sizeof(complex<T>) * size);
    }

    /// @brief Executes the direct (forward) stage.
    /// @param out Output buffer.
    /// @param in Input buffer.
    /// @param temp Scratch buffer (may be unused by the stage).
    KFR_MEM_INTRINSIC void execute(cdirect_t, complex<T>* out, const complex<T>* in, u8* temp)
    {
        do_execute(cdirect_t(), out, in, temp);
    }
    /// @brief Executes the inverse stage.
    /// @param out Output buffer.
    /// @param in Input buffer.
    /// @param temp Scratch buffer (may be unused by the stage).
    KFR_MEM_INTRINSIC void execute(cinvert_t, complex<T>* out, const complex<T>* in, u8* temp)
    {
        do_execute(cinvert_t(), out, in, temp);
    }
    /// @brief Executes the stage in the requested direction.
    /// @param inverse If true, executes the inverse stage; otherwise the direct stage.
    /// @param out Output buffer.
    /// @param in Input buffer.
    /// @param temp Scratch buffer (may be unused by the stage).
    KFR_MEM_INTRINSIC void execute(bool inverse, complex<T>* out, const complex<T>* in, u8* temp)
    {
        if (inverse)
            do_execute(cinvert_t(), out, in, temp);
        else
            do_execute(cdirect_t(), out, in, temp);
    }
    /// @brief Destructor.
    virtual ~dft_stage() {}

protected:
    /// @brief Initializes the stage's internal data for the given DFT size.
    virtual void do_initialize(size_t) {}
    /// @brief Performs the direct stage execution (implemented by derived stages).
    virtual void do_execute(cdirect_t, complex<T>*, const complex<T>*, u8* temp) = 0;
    /// @brief Performs the inverse stage execution (implemented by derived stages).
    virtual void do_execute(cinvert_t, complex<T>*, const complex<T>*, u8* temp) = 0;
};

/// @brief Direction(s) of DFT computation requested for a plan.
enum class dft_type
{
    both, ///< Both direct and inverse transforms.
    direct, ///< Only the direct (forward) transform.
    inverse, ///< Only the inverse transform.
};

/**
 * @brief Specifies the desired order for DFT output (and IDFT input).
 *
 * Currently ignored: the implementation always produces output in normal order
 * (any internal digit-reversal is handled transparently by the plan).
 */
enum class dft_order
{
    normal, ///< Normal order
    internal, ///< Possibly bit/digit-reversed, implementation-defined, may be faster to compute
};

/**
 * @brief Specifies the packing format for real DFT output data.
 * See https://www.kfr.dev/docs/latest/dft_format/ for details
 */
enum class dft_pack_format
{
    /// Packed format: {DC, Nyquist}, X[1], X[2], ..., X[N/2-1]
    /// Number of complex samples is $\frac{N}{2}$ where N is the number of real samples
    Perm,
    /// Conjugate-symmetric format: {DC, 0}, X[1], X[2], ..., X[N/2-1], {Nyquist, 0}
    /// Number of complex samples is $\frac{N}{2}+1$ where N is the number of real samples
    CCs,
};

template <typename T>
struct dft_plan;

template <typename T>
struct dft_plan_real;

template <typename T>
using dft_stage_ptr = std::unique_ptr<dft_stage<T>>;

namespace internal_generic
{
template <typename T>
void dft_initialize(dft_plan<T>& plan);
template <typename T>
void dft_real_initialize(dft_plan_real<T>& plan);
template <typename T, bool inverse>
void dft_execute(const dft_plan<T>& plan, cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp);

template <typename T>
using fn_transpose = void (*)(complex<T>*, const complex<T>*, shape<2>);
template <typename T>
void dft_initialize_transpose(fn_transpose<T>& transpose);

#ifdef KFR_CLASSIC_FFT
template <typename T>
void dft_progressive_start(const dft_plan<T>& plan, typename dft_plan<T>::progressive& progressive,
                           bool inverse, complex<T>* out, const complex<T>* in, u8* temp);
template <typename T>
void dft_progressive_step(const dft_plan<T>& plan, typename dft_plan<T>::progressive& progressive);
#endif

} // namespace internal_generic

/**
 * @brief Class for performing 1D DFT/FFT.
 *
 * The same plan is used for both direct DFT and inverse DFT. The type is default-constructible and movable
 * but non-copyable. It is advisable to create an instance of the `dft_plan` with a specific size
 * beforehand and reuse this instance in all subsequent DFT operations.
 *
 * @tparam T Template parameter specifying the floating-point type. Must be either `float` or `double`;
 *           other types are not supported.
 */
template <typename T>
struct dft_plan
{
    /// The size of the DFT as passed to the constructor.
    size_t size;

    /// The temporary (scratch) buffer size for the DFT plan.
    /// @note Preallocating a byte buffer of this size and passing its pointer to the
    /// `execute` function may improve performance.
    size_t temp_size;

    /**
     * @brief Constructs an empty DFT plan.
     *
     * This default constructor ensures the type is default-constructible.
     */
    dft_plan()
        : size(0), temp_size(0), data_size(0), arblen(false), disposition_inplace{}, disposition_outofplace{}
    {
    }

    /**
     * @brief Copy constructor (deleted).
     *
     * Copying of `dft_plan` instances is not allowed.
     */
    dft_plan(const dft_plan&) = delete;

    /**
     * @brief Copy assignment operator (deleted).
     *
     * Copy assignment of `dft_plan` instances is not allowed.
     */
    dft_plan& operator=(const dft_plan&) = delete;

    /**
     * @brief Move constructor.
     */
    dft_plan(dft_plan&&) = default;

    /**
     * @brief Move assignment operator.
     */
    dft_plan& operator=(dft_plan&&) = default;

    /**
     * @brief Checks whether the plan is non-empty.
     *
     * @return `true` if the plan was constructed with a specific DFT size, `false` otherwise.
     */
    bool is_initialized() const { return size != 0; }

    /**
     * @brief Constructs a DFT plan with the specified size and order.
     *
     * @param size The size of the DFT.
     * @param order The order of the DFT samples. See `dft_order` (currently ignored).
     * @param progressive_optimized If true, the plan will be optimized for progressive execution.
     */
    explicit dft_plan(size_t size, dft_order order = dft_order::normal, bool progressive_optimized = false)
        : size(size), temp_size(0), data_size(0), arblen(false), progressive_optimized(progressive_optimized)
    {
        internal_generic::dft_initialize(*this);
    }

    /**
     * @brief Dumps details of the DFT plan to stdout for inspection.
     *
     * May be used to determine the selected architecture at runtime and the chosen DFT algorithms.
     */
    void dump() const;

    /**
     * @brief Execute the complex DFT on `in` and write the result to `out`.
     * @param out Pointer to the output data.
     * @param in Pointer to the input data.
     * @param temp Temporary (scratch) buffer. If `nullptr` and `temp_size > 0`, a scratch
     * buffer of `temp_size` bytes is allocated (on stack or heap) for the duration of the call.
     * @param inverse If true, apply the inverse DFT.
     * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex
     * values to `out`, where $N$ is the size passed to the constructor.
     */
    KFR_MEM_INTRINSIC void execute(complex<T>* out, const complex<T>* in, u8* temp,
                                   bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out, in, temp);
        else
            execute_dft(cfalse, out, in, temp);
    }

    /**
     * @brief Destructor.
     *
     * Internal data (twiddle tables, stage objects) is released automatically through
     * the member destructors (`autofree`, `std::vector`, `std::unique_ptr`).
     */
    ~dft_plan() {}

    /**
     * @brief Execute the complex DFT on `in` and write the result to `out`.
     * @param out Pointer to the output data.
     * @param in Pointer to the input data.
     * @param temp Temporary (scratch) buffer. If `nullptr` and `temp_size > 0`, a scratch
     * buffer of `temp_size` bytes is allocated (on stack or heap) for the duration of the call.
     * @tparam inverse If true, apply the inverse DFT.
     * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex
     * values to `out`, where $N$ is the size passed to the constructor.
     */
    template <bool inverse>
    KFR_MEM_INTRINSIC void execute(complex<T>* out, const complex<T>* in, u8* temp,
                                   cbool_t<inverse> inv) const
    {
        execute_dft(inv, out, in, temp);
    }

    /**
     * @brief Execute the complex DFT on `in` and write the result to `out`.
     * @param out Output univector.
     * @param in Input univector.
     * @param temp Temporary (scratch) buffer univector. If its storage is `nullptr` and
     * `temp_size > 0`, a scratch buffer of `temp_size` bytes is allocated for the call.
     * @param inverse If true, apply the inverse DFT.
     * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex
     * values to `out`, where $N$ is the size passed to the constructor.
     */
    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   univector<u8, Tag3>& temp, bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out.data(), in.data(), temp.data());
        else
            execute_dft(cfalse, out.data(), in.data(), temp.data());
    }

    /**
     * @brief Execute the complex DFT on `in` and write the result to `out`.
     * @param out Output univector.
     * @param in Input univector.
     * @param temp Temporary (scratch) buffer univector. If its storage is `nullptr` and
     * `temp_size > 0`, a scratch buffer of `temp_size` bytes is allocated for the call.
     * @tparam inverse If true, apply the inverse DFT.
     * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex
     * values to `out`, where $N$ is the size passed to the constructor.
     */
    template <bool inverse, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   univector<u8, Tag3>& temp, cbool_t<inverse> inv) const
    {
        execute_dft(inv, out.data(), in.data(), temp.data());
    }

    /**
     * @brief Execute the complex DFT on `in` and write the result to `out`.
     * @param out Output univector.
     * @param in Input univector.
     * @param temp Temporary (scratch) buffer. If `nullptr` and `temp_size > 0`, a scratch
     * buffer of `temp_size` bytes is allocated (on stack or heap) for the duration of the call.
     * @param inverse If true, apply the inverse DFT.
     * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex
     * values to `out`, where $N$ is the size passed to the constructor.
     */
    template <univector_tag Tag1, univector_tag Tag2>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   u8* temp, bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out.data(), in.data(), temp);
        else
            execute_dft(cfalse, out.data(), in.data(), temp);
    }

    /**
     * @brief Execute the complex DFT on `in` and write the result to `out`.
     * @param out Output univector.
     * @param in Input univector.
     * @param temp Temporary (scratch) buffer. If `nullptr` and `temp_size > 0`, a scratch
     * buffer of `temp_size` bytes is allocated (on stack or heap) for the duration of the call.
     * @tparam inverse If true, apply the inverse DFT.
     * @note No scaling is applied. This function reads $N$ complex values from `in` and writes $N$ complex
     * values to `out`, where $N$ is the size passed to the constructor.
     */
    template <bool inverse, univector_tag Tag1, univector_tag Tag2>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   u8* temp, cbool_t<inverse> inv) const
    {
        execute_dft(inv, out.data(), in.data(), temp);
    }

    autofree<u8> data; /**< Internal data. */
    size_t data_size; /**< Internal data size. */

    std::vector<dft_stage_ptr<T>> all_stages; /**< Internal data. */
    std::array<std::vector<dft_stage<T>*>, 2> stages; /**< Internal data. */
    bool arblen; /**< True if Bluestein's FFT algorithm is selected. */
    bool progressive_optimized; /**< True if the plan is for progressive execution of the DFT. */
    using bitset = std::bitset<DFT_MAX_STAGES>; /**< Internal typedef. */
    std::array<bitset, 2> disposition_inplace; /**< Internal data. */
    std::array<bitset, 2> disposition_outofplace; /**< Internal data. */

    /// Internal function
    void calc_disposition();

    /// Internal function
    static bitset precompute_disposition(int num_stages, bitset can_inplace_per_stage,
                                         bool inplace_requested);

#ifdef KFR_CLASSIC_FFT
    /** Internal data structure for progressive execution of the DFT.
        Do not access the members directly as they may change in future versions.
     */
    struct progressive
    {
        bool inverse; ///< Direction of the transform.
        complex<T>* out; ///< Output buffer.
        const complex<T>* in; ///< Input buffer.
        u8* temp; ///< Scratch buffer.
        bitset disposition; ///< In-place/out-of-place disposition bitmask.
        complex<T>* scratch; ///< Internal scratch pointer.
        size_t step = 0; ///< Current step index.
    };

    /// @brief Returns the number of steps for progressive execution of the DFT.
    /// @return The number of steps for progressive execution.
    size_t progressive_total_steps() const;

    /**
     * @brief Initiates the progressive execution of the DFT.
     * @param inverse If true, applies the inverse DFT.
     * @param out Pointer to the output data.
     * @param in Pointer to the input data.
     * @param temp Temporary (scratch) buffer. A scratch buffer of size
     * `plan->temp_size` must be provided.
     * @return A `progressive` structure that can be used with `progressive_step`.
     * @note Ensure that the entire input data is available in the `in` buffer before calling this function.
     * The `out` buffer will contain the result data after the final step of the progressive execution.
     */
    KFR_MEM_INTRINSIC progressive progressive_start(bool inverse, complex<T>* out, const complex<T>* in,
                                                    u8* temp) const
    {
        KFR_LOGIC_CHECK(is_initialized(), "dft_plan is not initialized");
        KFR_LOGIC_CHECK(temp_size == 0 || temp != nullptr,
                        "Temporary buffer must be provided for progressive execution");
        progressive result{};
        internal_generic::dft_progressive_start(*this, result, inverse, out, in, temp);
        return result;
    }

    /**
     * @brief Steps the progressive execution of the DFT.
     * @param progressive A `progressive` structure returned by `progressive_start`.
     * @return `true` if there are more steps to execute, `false` if the DFT is complete.
     */
    KFR_MEM_INTRINSIC bool progressive_step(progressive& progressive) const
    {
        internal_generic::dft_progressive_step(*this, progressive);
        return ++progressive.step < stages[progressive.inverse].size();
    }
#endif

#ifdef KFR_DFT_MEASURE_STAGE_TIME
    /// @brief Resets the accumulated per-stage timing counters to zero.
    void reset_time()
    {
        for (auto& stage : all_stages)
        {
            stage->time = 0;
        }
    }

    /**
     * @brief Prints the accumulated per-stage timings to stdout.
     * @param invocations Number of invocations to average the timings over.
     * @param reset If true, resets the counters after printing.
     */
    void dump_times(uint64_t invocations = 1, bool reset = true)
    {
        double sum = 0;
        printf("DFT plan %zu\n", size);
        for (auto& stage : all_stages)
        {
            printf("  %s: %.3f us\n", stage->name ? stage->name : "unnamed", stage->time * 1e6 / invocations);
            sum += stage->time;
        }
        printf("Total: %.3f us\n", sum * 1e6 / invocations);
        if (reset)
            reset_time();
    }
#endif

protected:
    struct noinit
    {
    };
    explicit dft_plan(noinit, size_t size, dft_order order = dft_order::normal,
                      bool progressive_optimized = false)
        : size(size), temp_size(0), data_size(0), arblen(false), progressive_optimized(progressive_optimized)
    {
    }

    template <bool inverse>
    KFR_INTRINSIC void execute_dft(cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        internal_generic::dft_execute(*this, cbool<inverse>, out, in, temp);
    }
};

#define KFR_DFT_SUPPORTS_ODD_REAL 1

/**
 * @brief Real-to-complex and complex-to-real 1D DFT plan.
 *
 * Specializes `dft_plan<T>` for real-valued input/output. The forward transform
 * reads `size` real samples and produces `complex_size()` complex samples in the
 * chosen packed format. The inverse transform reads the packed spectrum and
 * produces `size` real samples. No scaling is applied in either direction.
 *
 * For odd sizes the transform is performed via a temporary complex buffer; for
 * even sizes the underlying complex DFT of size `size/2` is reused together with
 * a repacking stage.
 *
 * @tparam T Floating-point type (`float` or `double`).
 */
template <typename T>
struct dft_plan_real : dft_plan<T>
{
    size_t size; ///< Number of real samples in the transform.
    dft_pack_format fmt; ///< Packing format of the complex spectrum.

    /// @brief Constructs an empty (uninitialized) real DFT plan.
    dft_plan_real() : size(0), fmt(dft_pack_format::CCs) {}

    dft_plan_real(const dft_plan_real&)            = delete;
    dft_plan_real(dft_plan_real&&)                 = default;
    dft_plan_real& operator=(const dft_plan_real&) = delete;
    dft_plan_real& operator=(dft_plan_real&&)      = default;

    /// @brief Checks whether the plan is non-empty.
    bool is_initialized() const { return size != 0; }

    /// @brief Returns the number of complex samples produced/consumed.
    size_t complex_size() const { return complex_size_for(size, fmt); }
    /**
     * @brief Returns the number of complex samples for a given real size and format.
     * @param size Number of real samples.
     * @param fmt Packing format.
     * @return `size/2 + 1` for `CCs`, `(size+1)/2` for `Perm`.
     */
    constexpr static size_t complex_size_for(size_t size, dft_pack_format fmt)
    {
        return fmt == dft_pack_format::CCs ? size / 2 + 1 : (size + 1) / 2;
    }

    /**
     * @brief Constructs a real DFT plan.
     * @param size Number of real samples.
     * @param fmt Packing format of the complex spectrum.
     * @param progressive_optimized If true, optimize the plan for progressive execution.
     */
    explicit dft_plan_real(size_t size, dft_pack_format fmt = dft_pack_format::CCs,
                           bool progressive_optimized = false)
        : dft_plan<T>(typename dft_plan<T>::noinit{}, size % 2 ? size : size / 2, dft_order::normal,
                      progressive_optimized),
          size(size), fmt(fmt)
    {
        internal_generic::dft_real_initialize(*this);
    }

    void execute(complex<T>*, const complex<T>*, u8*, bool = false) const = delete;

    template <bool inverse>
    void execute(complex<T>*, const complex<T>*, u8*, cbool_t<inverse>) const = delete;

    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    void execute(univector<complex<T>, Tag1>&, const univector<complex<T>, Tag2>&, univector<u8, Tag3>&,
                 bool = false) const = delete;

    template <bool inverse, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    void execute(univector<complex<T>, Tag1>&, const univector<complex<T>, Tag2>&, univector<u8, Tag3>&,
                 cbool_t<inverse>) const = delete;

    template <univector_tag Tag1, univector_tag Tag2>
    void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in, u8* temp,
                 bool inverse = false) const = delete;

    template <bool inverse, univector_tag Tag1, univector_tag Tag2>
    void execute(univector<complex<T>, Tag1>& out, const univector<complex<T>, Tag2>& in, u8* temp,
                 cbool_t<inverse> inv) const = delete;

    /**
     * @brief Executes the forward real-to-complex DFT.
     * @param out Output complex buffer of `complex_size()` elements.
     * @param in Input real buffer of `size` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    KFR_MEM_INTRINSIC void execute(complex<T>* out, const T* in, u8* temp, cdirect_t = {}) const
    {
        if (this->size % 2 == 0)
        {
            this->execute_dft(cfalse, out, ptr_cast<complex<T>>(in), temp);
        }
        else
        {
            call_with_temp(sizeof(complex<T>) * this->size,
                           [&](u8* complex_buf)
                           {
                               complex<T>* tmp = ptr_cast<complex<T>>(complex_buf);
                               // Copy real input to temporary complex buffer with zero imaginary parts
                               process(make_univector(tmp, this->size), make_univector(in, this->size));

                               // Execute DFT on the temporary buffer
                               this->execute_dft(cfalse, tmp, tmp, temp);
                               // Copy the first N/2+1 values of result to the output buffer
                               const size_t csize = complex_size();
                               builtin_memcpy(out, tmp, sizeof(complex<T>) * csize);
                           });
        }
    }
    /**
     * @brief Executes the inverse complex-to-real DFT.
     * @param out Output real buffer of `size` elements.
     * @param in Input complex buffer of `complex_size()` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    KFR_MEM_INTRINSIC void execute(T* out, const complex<T>* in, u8* temp, cinvert_t = {}) const
    {
        if (this->size % 2 == 0)
        {
            this->execute_dft(ctrue, ptr_cast<complex<T>>(out), in, temp);
        }
        else
        {
            call_with_temp(sizeof(complex<T>) * this->size,
                           [&](u8* complex_buf)
                           {
                               complex<T>* tmp = ptr_cast<complex<T>>(complex_buf);
                               // Copy complex input to temporary complex buffer, reconstructing the second
                               // half of the spectrum
                               reconstruct_spectrum(tmp, in);
                               // Execute IDFT on the temporary buffer
                               this->execute_dft(ctrue, tmp, tmp, temp);
                               // Copy the real parts of the result to the output buffer
                               process(make_univector(out, this->size),
                                       real(make_univector(tmp, this->size)));
                           });
        }
    }

    /**
     * @brief Forward real-to-complex DFT using univector buffers with a scratch univector.
     * @param out Output complex univector of `complex_size()` elements.
     * @param in Input real univector of `size` elements.
     * @param temp Scratch buffer univector of at least `temp_size` bytes.
     */
    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<T, Tag2>& in,
                                   univector<u8, Tag3>& temp, cdirect_t = {}) const
    {
        this->execute(out.data(), in.data(), temp.data(), cdirect_t());
    }
    /**
     * @brief Inverse complex-to-real DFT using univector buffers with a scratch univector.
     * @param out Output real univector of `size` elements.
     * @param in Input complex univector of `complex_size()` elements.
     * @param temp Scratch buffer univector of at least `temp_size` bytes.
     */
    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<T, Tag1>& out, const univector<complex<T>, Tag2>& in,
                                   univector<u8, Tag3>& temp, cinvert_t = {}) const
    {
        this->execute(out.data(), in.data(), temp.data(), cinvert_t());
    }

    /**
     * @brief Forward real-to-complex DFT using univector buffers with a raw scratch pointer.
     * @param out Output complex univector of `complex_size()` elements.
     * @param in Input real univector of `size` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    template <univector_tag Tag1, univector_tag Tag2>
    KFR_MEM_INTRINSIC void execute(univector<complex<T>, Tag1>& out, const univector<T, Tag2>& in, u8* temp,
                                   cdirect_t = {}) const
    {
        this->execute(out.data(), in.data(), temp, cdirect_t());
    }
    /**
     * @brief Inverse complex-to-real DFT using univector buffers with a raw scratch pointer.
     * @param out Output real univector of `size` elements.
     * @param in Input complex univector of `complex_size()` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    template <univector_tag Tag1, univector_tag Tag2>
    KFR_MEM_INTRINSIC void execute(univector<T, Tag1>& out, const univector<complex<T>, Tag2>& in, u8* temp,
                                   cinvert_t = {}) const
    {
        this->execute(out.data(), in.data(), temp, cinvert_t());
    }

private:
    KFR_INTRINSIC void reconstruct_spectrum(complex<T>* tmp, const complex<T>* in) const
    {
        // Input: (N+1)/2 complex values (first half of Hermitian-symmetric spectrum)
        // Output: N complex values (full spectrum with conjugate symmetry)
        //   tmp[k] = in[k]           for k = 0 .. (N-1)/2
        //   tmp[k] = conj(in[N-k])   for k = (N+1)/2 .. N-1
        const size_t csize = complex_size(); // (N+1)/2
        builtin_memcpy(tmp, in, sizeof(complex<T>) * csize);
        // Reconstruct the second half using Hermitian symmetry: X[N-k] = conj(X[k])
        // For k from 1 to (N-1)/2, set tmp[N-k] = conj(in[k])
        process(make_univector(tmp + csize, csize - 1), reverse(cconj(make_univector(in + 1, csize - 1))));
    }

public:
#ifdef KFR_CLASSIC_FFT
    using progressive = typename dft_plan<T>::progressive;

    /**
     * @brief Initiates progressive execution of the inverse (complex-to-real) transform.
     * @param out Output real buffer of `size` elements (filled after the final step).
     * @param in Input complex buffer of `complex_size()` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (must not be `nullptr` if `temp_size > 0`).
     * @return A `progressive` state to advance with `progressive_step`.
     */
    KFR_MEM_INTRINSIC progressive progressive_start(T* out, const complex<T>* in, u8* temp) const
    {
        KFR_LOGIC_CHECK(is_initialized(), "dft_plan_real is not initialized");
        KFR_LOGIC_CHECK(this->temp_size == 0 || temp != nullptr,
                        "Temporary buffer must be provided for progressive execution");
        progressive result{};
        internal_generic::dft_progressive_start(*this, result, true, ptr_cast<complex<T>>(out), in, temp);
        return result;
    }
    /**
     * @brief Initiates progressive execution of the forward (real-to-complex) transform.
     * @param out Output complex buffer of `complex_size()` elements (filled after the final step).
     * @param in Input real buffer of `size` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (must not be `nullptr` if `temp_size > 0`).
     * @return A `progressive` state to advance with `progressive_step`.
     */
    KFR_MEM_INTRINSIC progressive progressive_start(complex<T>* out, const T* in, u8* temp) const
    {
        KFR_LOGIC_CHECK(is_initialized(), "dft_plan_real is not initialized");
        KFR_LOGIC_CHECK(this->temp_size == 0 || temp != nullptr,
                        "Temporary buffer must be provided for progressive execution");
        progressive result{};
        internal_generic::dft_progressive_start(*this, result, false, out, ptr_cast<const complex<T>>(in),
                                                temp);
        return result;
    }
#endif
};

/**
 * @brief Multidimensional complex DFT plan.
 *
 * Computes a separable DFT over all dimensions by applying a 1D `dft_plan<T>`
 * along each axis, transposing the data between axes so that each axis is
 * processed with unit stride. The same plan supports both forward and inverse
 * transforms; no scaling is applied.
 *
 * @tparam T Floating-point type (`float` or `double`).
 * @tparam Dims Number of dimensions, or `dynamic_shape` for a runtime rank.
 */
template <typename T, index_t Dims = dynamic_shape>
struct dft_plan_md
{
    shape<Dims> size; ///< Per-dimension sizes of the transform.
    size_t temp_size; ///< Scratch buffer size in bytes required by `execute`.

    dft_plan_md(const dft_plan_md&)            = delete;
    dft_plan_md(dft_plan_md&&)                 = default;
    dft_plan_md& operator=(const dft_plan_md&) = delete;
    dft_plan_md& operator=(dft_plan_md&&)      = default;

    /// @brief Checks whether the plan is non-empty.
    bool is_initialized() const { return size.product() != 0; }

    /// @brief Dumps details of the underlying per-axis plans to stdout.
    void dump() const
    {
        for (const auto& d : dfts)
        {
            d.dump();
        }
    }

    /// @brief Constructs a multidimensional DFT plan for the given shape.
    /// @param size Per-dimension sizes of the transform.
    explicit dft_plan_md(shape<Dims> size) : size(std::move(size)), temp_size(0)
    {
        if constexpr (Dims == dynamic_shape)
        {
            dfts.resize(this->size.dims());
        }
        for (index_t i = 0; i < this->size.dims(); ++i)
        {
            dfts[i]   = dft_plan<T>(this->size[i]);
            temp_size = std::max(temp_size, dfts[i].temp_size);
        }
        internal_generic::dft_initialize_transpose(transpose);
    }

    /**
     * @brief Executes the multidimensional DFT on raw pointers.
     * @param out Output buffer of `size.product()` complex elements.
     * @param in Input buffer of `size.product()` complex elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     * @param inverse If true, performs the inverse transform.
     */
    void execute(complex<T>* out, const complex<T>* in, u8* temp, bool inverse = false) const
    {
        if (inverse)
            execute_dft(ctrue, out, in, temp);
        else
            execute_dft(cfalse, out, in, temp);
    }

    /**
     * @brief Executes the multidimensional DFT on tensors.
     * @param out Output tensor with shape equal to `size`.
     * @param in Input tensor with shape equal to `size`.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     * @param inverse If true, performs the inverse transform.
     */
    void execute(const tensor<complex<T>, Dims>& out, const tensor<complex<T>, Dims>& in, u8* temp,
                 bool inverse = false) const
        requires(Dims != dynamic_shape)
    {
        KFR_LOGIC_CHECK(in.shape() == this->size && out.shape() == this->size,
                        "dft_plan_md: incorrect tensor shapes");
        KFR_LOGIC_CHECK(in.is_contiguous() && out.is_contiguous(), "dft_plan_md: tensors must be contiguous");
        if (inverse)
            execute_dft(ctrue, out.data(), in.data(), temp);
        else
            execute_dft(cfalse, out.data(), in.data(), temp);
    }
    /**
     * @brief Executes the multidimensional DFT with a compile-time direction.
     * @tparam inverse If true, performs the inverse transform.
     * @param out Output buffer of `size.product()` complex elements.
     * @param in Input buffer of `size.product()` complex elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    template <bool inverse = false>
    void execute(complex<T>* out, const complex<T>* in, u8* temp, cbool_t<inverse> = {}) const
    {
        execute_dft(cbool<inverse>, out, in, temp);
    }

private:
    template <bool inverse>
    KFR_INTRINSIC void execute_dft(cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        if (temp == nullptr && temp_size > 0)
        {
            return call_with_temp(temp_size, std::bind(&dft_plan_md<T, Dims>::execute_dft<inverse>, this,
                                                       cbool_t<inverse>{}, out, in, std::placeholders::_1));
        }
        if (size.dims() == 1)
        {
            dfts[0].execute(out, in, temp, cbool<inverse>);
        }
        else
        {
            execute_dim(cbool<inverse>, out, in, temp);
        }
    }
    KFR_INTRINSIC void execute_dim(cfalse_t, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        shape<Dims> sh = size;
        index_t total  = size.product();
        index_t axis   = size.dims() - 1;
        for (;;)
        {
            if (size[axis] > 1)
            {
                for (index_t o = 0; o < total; o += sh.back())
                    dfts[axis].execute(out + o, in + o, temp, cfalse);
            }
            else
            {
                builtin_memcpy(out, in, sizeof(complex<T>) * total);
            }

            transpose(out, out, shape{ sh.remove_back().product(), sh.back() });

            if (axis == 0)
                break;

            sh = sh.rotate_right();
            in = out;
            --axis;
        }
    }
    KFR_INTRINSIC void execute_dim(ctrue_t, complex<T>* out, const complex<T>* in, u8* temp) const
    {
        shape<Dims> sh = size;
        index_t total  = size.product();
        index_t axis   = 0;
        for (;;)
        {
            transpose(out, in, shape{ sh.front(), sh.remove_front().product() });

            if (size[axis] > 1)
            {
                for (index_t o = 0; o < total; o += sh.front())
                    dfts[axis].execute(out + o, out + o, temp, ctrue);
            }

            if (axis == size.dims() - 1)
                break;

            sh = sh.rotate_left();
            in = out;
            ++axis;
        }
    }
    using dft_list =
        std::conditional_t<Dims == dynamic_shape, std::vector<dft_plan<T>>, std::array<dft_plan<T>, Dims>>;
    dft_list dfts;
    internal_generic::fn_transpose<T> transpose;
};

/**
 * @brief Multidimensional real-to-complex / complex-to-real DFT plan.
 *
 * The forward transform reads a real tensor of shape `size` and produces a
 * complex tensor of shape `complex_size()` (Hermitian-symmetric along the last
 * axis). The inverse transform reads the packed complex tensor and produces the
 * real tensor. No scaling is applied.
 *
 * @tparam T Floating-point type (`float` or `double`).
 * @tparam Dims Number of dimensions, or `dynamic_shape` for a runtime rank.
 */
template <typename T, index_t Dims = dynamic_shape>
struct dft_plan_md_real
{
    shape<Dims> size; ///< Per-dimension sizes of the real transform.
    size_t temp_size; ///< Scratch buffer size in bytes required by `execute`.
    /// @brief If true, the inverse transform may write directly into the real
    ///        output buffer without an extra complex working region.
    bool real_out_is_enough;

    dft_plan_md_real(const dft_plan_md_real&)            = delete;
    dft_plan_md_real(dft_plan_md_real&&)                 = default;
    dft_plan_md_real& operator=(const dft_plan_md_real&) = delete;
    dft_plan_md_real& operator=(dft_plan_md_real&&)      = default;

    /// @brief Checks whether the plan is non-empty.
    bool is_initialized() const { return size.product() != 0; }

    /// @brief Dumps details of the underlying per-axis plans to stdout.
    void dump() const
    {
        for (const auto& d : dfts)
        {
            d.dump();
        }
        dft_real.dump();
    }

    /// @brief Returns the shape of the complex (packed) spectrum.
    shape<Dims> complex_size() const { return complex_size_for(size); }
    /// @brief Returns the complex spectrum shape for a given real shape.
    constexpr static shape<Dims> complex_size_for(shape<Dims> size)
    {
        if (size.dims() > 0)
            size.back() = dft_plan_real<T>::complex_size_for(size.back(), dft_pack_format::CCs);
        return size;
    }

    /// @brief Returns the number of real elements needed to hold the inverse output.
    size_t real_out_size() const { return real_out_size_for(size); }
    /// @brief Returns the real output element count for a given real shape.
    constexpr static size_t real_out_size_for(shape<Dims> size)
    {
        return complex_size_for(size).product() * 2;
    }

    /**
     * @brief Constructs a multidimensional real DFT plan.
     * @param size Per-dimension sizes of the real transform.
     * @param real_out_is_enough If true, the inverse transform may write directly
     *        into the real output buffer without an extra complex working region.
     */
    explicit dft_plan_md_real(shape<Dims> size, bool real_out_is_enough = false)
        : size(std::move(size)), temp_size(0), real_out_is_enough(real_out_is_enough)
    {
        if (this->size.dims() > 0)
        {
            if constexpr (Dims == dynamic_shape)
            {
                dfts.resize(this->size.dims());
            }
            for (index_t i = 0; i < this->size.dims() - 1; ++i)
            {
                dfts[i]   = dft_plan<T>(this->size[i]);
                temp_size = std::max(temp_size, dfts[i].temp_size);
            }
            dft_real  = dft_plan_real<T>(this->size.back());
            temp_size = std::max(temp_size, dft_real.temp_size);
        }
        if (!this->real_out_is_enough)
        {
            temp_size += complex_size().product() * sizeof(complex<T>);
        }
        internal_generic::dft_initialize_transpose(transpose);
    }

    /**
     * @brief Forward transform: real input to packed complex output (raw pointers).
     * @param out Output complex buffer of `complex_size().product()` elements.
     * @param in Input real buffer of `size.product()` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    void execute(complex<T>* out, const T* in, u8* temp, cdirect_t = {}) const
    {
        execute_dft(cfalse, out, in, temp);
    }
    /**
     * @brief Inverse transform: packed complex input to real output (raw pointers).
     * @param out Output real buffer of `size.product()` elements.
     * @param in Input complex buffer of `complex_size().product()` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    void execute(T* out, const complex<T>* in, u8* temp, cinvert_t = {}) const
    {
        execute_dft(ctrue, out, in, temp);
    }
    /**
     * @brief Forward transform on tensors.
     * @param out Output complex tensor with shape `complex_size()`.
     * @param in Input real tensor with shape `size`.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    void execute(const tensor<complex<T>, Dims>& out, const tensor<T, Dims>& in, u8* temp,
                 cdirect_t = {}) const
        requires(Dims != dynamic_shape)
    {
        KFR_LOGIC_CHECK(in.shape() == this->size && out.shape() == complex_size(),
                        "dft_plan_md_real: incorrect tensor shapes");
        KFR_LOGIC_CHECK(in.is_contiguous() && out.is_contiguous(),
                        "dft_plan_md_real: tensors must be contiguous");
        execute_dft(cfalse, out.data(), in.data(), temp);
    }
    /**
     * @brief Inverse transform on tensors.
     * @param out Output real tensor with shape `size`.
     * @param in Input complex tensor with shape `complex_size()`.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     */
    void execute(const tensor<T, Dims>& out, const tensor<complex<T>, Dims>& in, u8* temp,
                 cinvert_t = {}) const
        requires(Dims != dynamic_shape)
    {
        KFR_LOGIC_CHECK(in.shape() == complex_size() && out.shape() == this->size,
                        "dft_plan_md_real: incorrect tensor shapes");
        KFR_LOGIC_CHECK(in.is_contiguous() && out.is_contiguous(),
                        "dft_plan_md_real: tensors must be contiguous");
        execute_dft(ctrue, out.data(), in.data(), temp);
    }
    /**
     * @brief Forward transform selected by a runtime flag (raw pointers).
     * @param out Output complex buffer of `complex_size().product()` elements.
     * @param in Input real buffer of `size.product()` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     * @param inverse Must be `false` for the forward transform.
     */
    void execute(complex<T>* out, const T* in, u8* temp, bool inverse) const
    {
        KFR_LOGIC_CHECK(!inverse, "dft_plan_md_real: incorrect usage");
        execute_dft(cfalse, out, in, temp);
    }
    /**
     * @brief Inverse transform selected by a runtime flag (raw pointers).
     * @param out Output real buffer of `size.product()` elements.
     * @param in Input complex buffer of `complex_size().product()` elements.
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     * @param inverse Must be `true` for the inverse transform.
     */
    void execute(T* out, const complex<T>* in, u8* temp, bool inverse) const
    {
        KFR_LOGIC_CHECK(inverse, "dft_plan_md_real: incorrect usage");
        execute_dft(ctrue, out, in, temp);
    }

private:
    template <bool inverse, typename Tout, typename Tin>
    KFR_INTRINSIC void execute_dft(cbool_t<inverse>, Tout* out, const Tin* in, u8* temp) const
    {
        if (temp == nullptr && temp_size > 0)
        {
            return call_with_temp(temp_size,
                                  std::bind(&dft_plan_md_real<T, Dims>::execute_dft<inverse, Tout, Tin>, this,
                                            cbool_t<inverse>{}, out, in, std::placeholders::_1));
        }
        if (this->size.dims() == 1)
        {
            dft_real.execute(out, in, temp, cbool<inverse>);
        }
        else
        {
            execute_dim(cbool<inverse>, out, in, temp);
        }
    }
    void expand(T* out, const T* in, size_t count, size_t last_axis) const
    {
        size_t last_axis_ex = dft_real.complex_size() * 2;
        if (in != out)
        {
            builtin_memmove(out, in, last_axis * sizeof(T));
        }
        in += last_axis * (count - 1);
        out += last_axis_ex * (count - 1);
        for (size_t i = 1; i < count; ++i)
        {
            builtin_memmove(out, in, last_axis * sizeof(T));
            in -= last_axis;
            out -= last_axis_ex;
        }
#ifdef KFR_DEBUG
        for (size_t i = 0; i < count; ++i)
        {
            builtin_memset(out + last_axis, 0xFF, (last_axis_ex - last_axis) * sizeof(T));
            out += last_axis_ex;
        }
#endif
    }
    void contract(T* out, const T* in, size_t count, size_t last_axis) const
    {
        size_t last_axis_ex = dft_real.complex_size() * 2;
        if (in != out)
            builtin_memmove(out, in, last_axis * sizeof(T));
        in += last_axis_ex;
        out += last_axis;
        for (size_t i = 1; i < count; ++i)
        {
            builtin_memmove(out, in, last_axis * sizeof(T));
            in += last_axis_ex;
            out += last_axis;
        }
    }
    KFR_INTRINSIC void execute_dim(cfalse_t, complex<T>* out, const T* in_real, u8* temp) const
    {
        shape<Dims> sh = complex_size();
        index_t total  = sh.product();
        index_t axis   = size.dims() - 1;
        expand(ptr_cast<T>(out), in_real, size.remove_back().product(), size.back());
        for (;;)
        {
            if (size[axis] > 1)
            {
                if (axis == size.dims() - 1)
                    for (index_t o = 0; o < total; o += sh.back())
                        dft_real.execute(out + o, ptr_cast<T>(out + o), temp, cfalse);
                else
                    for (index_t o = 0; o < total; o += sh.back())
                        dfts[axis].execute(out + o, out + o, temp, cfalse);
            }

            transpose(out, out, shape{ sh.remove_back().product(), sh.back() });

            if (axis == 0)
                break;

            sh = sh.rotate_right();
            --axis;
        }
    }
    KFR_INTRINSIC void execute_dim(ctrue_t, T* out_real, const complex<T>* in, u8* temp) const
    {
        shape<Dims> sh  = complex_size();
        index_t total   = sh.product();
        complex<T>* out = real_out_is_enough
                              ? ptr_cast<complex<T>>(out_real)
                              : ptr_cast<complex<T>>(temp + temp_size - total * sizeof(complex<T>));
        index_t axis    = 0;
        for (;;)
        {
            transpose(out, in, shape{ sh.front(), sh.remove_front().product() });

            if (size[axis] > 1)
            {
                if (axis == size.dims() - 1)
                    for (index_t o = 0; o < total; o += sh.front())
                        dft_real.execute(ptr_cast<T>(out + o), out + o, temp, ctrue);
                else
                    for (index_t o = 0; o < total; o += sh.front())
                        dfts[axis].execute(out + o, out + o, temp, ctrue);
            }

            if (axis == size.dims() - 1)
                break;

            sh = sh.rotate_left();
            in = out;
            ++axis;
        }
        contract(out_real, ptr_cast<T>(out), size.remove_back().product(), size.back());
    }
    using dft_list = std::conditional_t<Dims == dynamic_shape, std::vector<dft_plan<T>>,
                                        std::array<dft_plan<T>, std::max(Dims, index_t(1)) - 1>>;
    dft_list dfts;
    dft_plan_real<T> dft_real;
    internal_generic::fn_transpose<T> transpose;
};

/**
 * @brief Plan for computing the Discrete Cosine Transform (DCT type 2, unscaled).
 *
 * The forward transform computes an unscaled DCT-II; the inverse transform
 * computes the corresponding DCT-III (the transpose/inverse of DCT-II, also
 * unscaled). Both directions are implemented by mirroring the real input into
 * a complex buffer and reusing the complex `dft_plan` machinery.
 *
 * @note No scaling is applied in either direction. Apply the conventional
 *       $\frac{2}{N}$ (or orthonormal) factor yourself if a normalized DCT is
 *       required.
 *
 * @tparam T Floating-point type (`float` or `double`).
 */
template <typename T>
struct dct_plan : dft_plan<T>
{
    /// @brief Constructs a DCT plan for the given size.
    /// @param size Number of real samples in the transform.
    dct_plan(size_t size) : dft_plan<T>(size) { this->temp_size += sizeof(complex<T>) * size * 2; }

    /**
     * @brief Executes the DCT.
     * @param out Output real buffer (size elements).
     * @param in Input real buffer (size elements).
     * @param temp Scratch buffer of at least `temp_size` bytes (may be `nullptr`).
     * @param inverse If true, computes the inverse (DCT-III); otherwise DCT-II.
     */
    KFR_MEM_INTRINSIC void execute(T* out, const T* in, u8* temp, bool inverse = false) const
    {
        const size_t size                  = this->size;
        const size_t halfSize              = size / 2;
        univector_ref<complex<T>> mirrored = make_univector(
            ptr_cast<complex<T>>(temp + this->temp_size - sizeof(complex<T>) * size * 2), size);
        univector_ref<complex<T>> mirrored_dft =
            make_univector(ptr_cast<complex<T>>(temp + this->temp_size - sizeof(complex<T>) * size), size);
        auto t = counter() * c_pi<T> / (size * 2);
        if (!inverse)
        {
            for (size_t i = 0; i < halfSize; i++)
            {
                mirrored[i]            = in[i * 2];
                mirrored[size - 1 - i] = in[i * 2 + 1];
            }
            if (size % 2)
            {
                mirrored[halfSize] = in[size - 1];
            }
            dft_plan<T>::execute(mirrored_dft.data(), mirrored.data(), temp, cfalse);
            make_univector(out, size) = real(mirrored_dft) * cos(t) + imag(mirrored_dft) * sin(t);
        }
        else
        {
            mirrored    = make_complex(make_univector(in, size) * cos(t), make_univector(in, size) * -sin(t));
            mirrored[0] = mirrored[0] * T(0.5);
            dft_plan<T>::execute(mirrored_dft.data(), mirrored.data(), temp, cfalse);
            for (size_t i = 0; i < halfSize; i++)
            {
                out[i * 2 + 0] = mirrored_dft[i].real();
                out[i * 2 + 1] = mirrored_dft[size - 1 - i].real();
            }
            if (size % 2)
            {
                out[size - 1] = mirrored_dft[halfSize].real();
            }
        }
    }

    /**
     * @brief Executes the DCT using univector buffers.
     * @param out Output real univector (size elements).
     * @param in Input real univector (size elements).
     * @param temp Scratch buffer univector of at least `temp_size` bytes.
     * @param inverse If true, computes the inverse (DCT-III); otherwise DCT-II.
     */
    template <univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
    KFR_MEM_INTRINSIC void execute(univector<T, Tag1>& out, const univector<T, Tag2>& in,
                                   univector<u8, Tag3>& temp, bool inverse = false) const
    {
        execute(out.data(), in.data(), temp.data(), inverse);
    }
};

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Element-wise multiplication of two spectra (convolution in time domain).
 *
 * Computes `dest = src1 * src2` element-wise, with special handling of the
 * DC/Nyquist bin when using the `Perm` packed format (where DC and Nyquist are
 * packed together in a single complex value).
 *
 * @param dest Destination spectrum (also the result).
 * @param src1 First operand spectrum.
 * @param src2 Second operand spectrum.
 * @param fmt Packing format of the spectra; only `Perm` triggers special bin-0 handling.
 */
template <typename T, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
void fft_multiply(univector<complex<T>, Tag1>& dest, const univector<complex<T>, Tag2>& src1,
                  const univector<complex<T>, Tag3>& src2, dft_pack_format fmt = dft_pack_format::CCs)
{
    const complex<T> f0(src1[0].real() * src2[0].real(), src1[0].imag() * src2[0].imag());

    dest = src1 * src2;

    if (fmt == dft_pack_format::Perm)
        dest[0] = f0;
}

/**
 * @brief Multiply-accumulate of two spectra into a destination.
 *
 * Computes `dest = dest + src1 * src2` element-wise, with special handling of
 * the DC/Nyquist bin for the `Perm` packed format.
 *
 * @param dest Destination/accumulator spectrum.
 * @param src1 First operand spectrum.
 * @param src2 Second operand spectrum.
 * @param fmt Packing format of the spectra; only `Perm` triggers special bin-0 handling.
 */
template <typename T, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3>
void fft_multiply_accumulate(univector<complex<T>, Tag1>& dest, const univector<complex<T>, Tag2>& src1,
                             const univector<complex<T>, Tag3>& src2,
                             dft_pack_format fmt = dft_pack_format::CCs)
{
    const complex<T> f0(dest[0].real() + src1[0].real() * src2[0].real(),
                        dest[0].imag() + src1[0].imag() * src2[0].imag());

    dest = dest + src1 * src2;

    if (fmt == dft_pack_format::Perm)
        dest[0] = f0;
}
/**
 * @brief Multiply-accumulate of two spectra with an addend into a destination.
 *
 * Computes `dest = src1 + src2 * src3` element-wise, with special handling of
 * the DC/Nyquist bin for the `Perm` packed format.
 *
 * @param dest Destination/accumulator spectrum.
 * @param src1 Addend spectrum.
 * @param src2 First factor spectrum.
 * @param src3 Second factor spectrum.
 * @param fmt Packing format of the spectra; only `Perm` triggers special bin-0 handling.
 */
template <typename T, univector_tag Tag1, univector_tag Tag2, univector_tag Tag3, univector_tag Tag4>
void fft_multiply_accumulate(univector<complex<T>, Tag1>& dest, const univector<complex<T>, Tag2>& src1,
                             const univector<complex<T>, Tag3>& src2, const univector<complex<T>, Tag4>& src3,
                             dft_pack_format fmt = dft_pack_format::CCs)
{
    const complex<T> f0(src1[0].real() + src2[0].real() * src3[0].real(),
                        src1[0].imag() + src2[0].imag() * src3[0].imag());

    dest = src1 + src2 * src3;

    if (fmt == dft_pack_format::Perm)
        dest[0] = f0;
}
} // namespace KFR_ARCH_NAME

/**
 * @brief Plan structure for the ng FFT algorithms (new in KFR 7.1).
 *
 * The ngFFT family provides lightweight, in-place FFT computation that requires
 * only a pre-computed twiddle factor table and no additional scratch (temporary)
 * buffer. This makes it suitable for environments where memory allocations must
 * be minimised or fully controlled by the caller.
 *
 * @par Key characteristics:
 *  - **Always in-place** — the input buffer is overwritten with the result.
 *  - **No scratch buffer required** — unlike the general-purpose @ref dft_plan,
 *    ngFFT does not allocate or need a temporary working buffer.
 *  - **Twiddle-initialisation only** — after setting @ref l2fftsize, the caller
 *    allocates a twiddle buffer (cache-line aligned), queries its required size
 *    via @ref ngfft_twiddle_count(), and initialises it with @ref ngfft_initialize().
 *    The twiddle buffer can be reused across executions for the same FFT size
 *    and algorithm.
 *
 * @par Lifetime & ownership:
 * The plan does **not** own the twiddle buffer. The caller is responsible for
 * allocating (with sufficient alignment) and deallocating it.
 *
 * @par Usage example:
 * @code{.cpp}
 * ngfft_plan<float> plan{ 16 };          // log2 of FFT size → 2^16 = 65536
 * plan.twiddles = aligned_allocate<complex<float>>(ngfft_twiddle_count(plan));
 * ngfft_initialize(plan);                // precompute twiddle factors
 *
 * ngfft_execute(plan, false, data);      // forward FFT, in-place
 *
 * aligned_deallocate(plan.twiddles);     // free twiddle buffer when done
 * @endcode
 *
 * @tparam T Floating-point scalar type (float or double).
 *
 * @see ngfft_twiddle_count()
 * @see ngfft_initialize()
 * @see ngfft_execute()
 * @see dft_algorithm
 */
template <typename T>
struct ngfft_plan
{
    /// Log2 of the FFT size (e.g. 16 means a 65536-point FFT).
    uint8_t l2fftsize;
    /// User-allocated pointer to twiddle factors. The required element count
    /// is obtained from ngfft_twiddle_count(); the buffer must be cache-line
    /// aligned. Initialise via ngfft_initialize() before first use.
    complex<T>* twiddles = nullptr;
};

template <typename T>
struct ngfft_plan_real : public ngfft_plan<T>
{
    ngfft_plan_real() noexcept = default;
    ngfft_plan_real(uint8_t l2fftsize) noexcept { this->l2fftsize = l2fftsize - 1; }
};

/// @brief Family of DFT algorithms (groups related algorithms).
enum class dft_family
{
    fourstep, ///< Four-step FFT family.
};

/// @brief Concrete DFT/FFT algorithm selection.
enum class dft_algorithm
{
    fourstep, ///< Four-step FFT algorithm.
};

constexpr inline dft_algorithm default_dft_algorithm = dft_algorithm::fourstep;

/// @brief Decomposition direction for a butterfly pass.
enum class dft_decomp : uint8_t
{
    dif, ///< Decimation-in-frequency
    dit, ///< Decimation-in-time
};

namespace internal_generic
{
/**
 * @brief Returns the number of twiddle-factor elements required by the plan.
 *
 * The returned count is the size (in `complex<T>` elements) of the twiddle
 * buffer that must be allocated and passed to `ngfft_initialize()`.
 *
 * @tparam T Floating-point scalar type.
 * @tparam algo FFT algorithm.
 * @param plan The plan whose `l2fftsize` is log2(fftsize).
 * @return Number of twiddle elements, or `SIZE_MAX` for an invalid FFT size.
 */
template <typename T, dft_algorithm algo>
size_t ngfft_twiddle_count(ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>);

/**
 * @brief Precomputes the twiddle factors into the plan's `twiddles` buffer.
 *
 * The caller must have allocated `plan.twiddles` with at least
 * `ngfft_twiddle_count()` elements before calling this function.
 *
 * @tparam T Floating-point scalar type.
 * @tparam algo FFT algorithm.
 * @param plan The plan to initialize; `plan.twiddles` must point to a valid buffer.
 * @return `true` on success; `false` if the FFT size is invalid or the required
 *         twiddle buffer was not provided.
 */
template <typename T, dft_algorithm algo>
bool ngfft_initialize(ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>);

/**
 * @brief Executes the FFT with separate input and output buffers.
 *
 * The transform reads from the input buffer and writes the result to the output
 * buffer. No scaling is applied.
 *
 * @tparam T Floating-point scalar type.
 * @tparam algo FFT algorithm.
 * @tparam inverse If true, performs the inverse FFT; otherwise the forward FFT.
 * @param plan The initialized plan.
 * @param out Output buffer of `2^plan.l2fftsize` complex elements.
 * @param in Input buffer of `2^plan.l2fftsize` complex elements.
 */
template <typename T, dft_algorithm algo, bool inverse>
void ngfft_execute(const ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>, cbool_t<inverse>, complex<T>* out,
                   const complex<T>* in);

template <typename T, dft_algorithm algo>
void ngfft_real_execute(const ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>, complex<T>* out, const T* in);

template <typename T, dft_algorithm algo>
void ngfft_real_execute(const ngfft_plan<T>& plan, cval_t<dft_algorithm, algo>, T* out, const complex<T>* in);

#define KFR_DFT_ALGO_SWITCH(algo, ...)                                                                       \
    switch (algo)                                                                                            \
    {                                                                                                        \
    case dft_algorithm::fourstep:                                                                            \
    {                                                                                                        \
        constexpr cval_t<dft_algorithm, dft_algorithm::fourstep> calg{};                                     \
        __VA_ARGS__                                                                                          \
    }                                                                                                        \
    default:                                                                                                 \
        KFR_UNREACHABLE;                                                                                     \
    }
} // namespace internal_generic

/**
 * @brief Returns the twiddle-factor count for a runtime algorithm value.
 * @param plan The plan whose `l2fftsize` selects the specialization.
 * @param algo FFT algorithm (defaults to `fourstep`).
 * @return Number of twiddle elements, or `SIZE_MAX` for an invalid FFT size.
 */
template <typename T>
inline size_t ngfft_twiddle_count(ngfft_plan<T>& plan, dft_algorithm algo = default_dft_algorithm)
{
    KFR_DFT_ALGO_SWITCH(algo, return internal_generic::ngfft_twiddle_count(plan, calg););
}

/**
 * @brief Initializes the plan's twiddles for a runtime algorithm value.
 * @param plan The plan to initialize; `plan.twiddles` must point to a valid buffer.
 * @param algo FFT algorithm (defaults to `fourstep`).
 * @return `true` on success; `false` if the FFT size is invalid or the required
 *         twiddle buffer was not provided.
 */
template <typename T>
inline bool ngfft_initialize(ngfft_plan<T>& plan, dft_algorithm algo = default_dft_algorithm)
{
    KFR_DFT_ALGO_SWITCH(algo, return internal_generic::ngfft_initialize(plan, calg););
}

/**
 * @brief Executes the FFT in-place with a compile-time direction and runtime algorithm.
 * @tparam inverse If true, performs the inverse FFT; otherwise the forward FFT.
 * @param plan The initialized plan.
 * @param inout Input/output buffer of `2^plan.l2fftsize` complex elements.
 * @param algo FFT algorithm (defaults to `fourstep`).
 */
template <typename T, bool inverse>
KFR_INLINE void ngfft_execute(const ngfft_plan<T>& plan, cbool_t<inverse>, complex<T>* inout,
                              dft_algorithm algo = default_dft_algorithm)
{
    KFR_DFT_ALGO_SWITCH(
        algo, return internal_generic::ngfft_execute(plan, calg, cbool_t<inverse>(), inout, inout););
}

/**
 * @brief Executes the FFT with separate input and output buffers, a compile-time direction and runtime
 * algorithm.
 * @tparam inverse If true, performs the inverse FFT; otherwise the forward FFT.
 * @param plan The initialized plan.
 * @param out Output buffer of `2^plan.l2fftsize` complex elements.
 * @param in Input buffer of `2^plan.l2fftsize` complex elements.
 * @param algo FFT algorithm (defaults to `fourstep`).
 */
template <typename T, bool inverse>
KFR_INLINE void ngfft_execute(const ngfft_plan<T>& plan, cbool_t<inverse>, complex<T>* out,
                              const complex<T>* in, dft_algorithm algo = default_dft_algorithm)
{
    KFR_DFT_ALGO_SWITCH(algo,
                        return internal_generic::ngfft_execute(plan, calg, cbool_t<inverse>(), out, in););
}

/**
 * @brief Executes the FFT in-place with runtime direction and algorithm.
 * @param plan The initialized plan.
 * @param inverse If true, performs the inverse FFT; otherwise the forward FFT.
 * @param inout Input/output buffer of `2^plan.l2fftsize` complex elements.
 * @param algo FFT algorithm (defaults to `fourstep`).
 */
template <typename T>
KFR_INLINE void ngfft_execute(const ngfft_plan<T>& plan, bool inverse, complex<T>* inout,
                              dft_algorithm algo = default_dft_algorithm)
{
    KFR_DFT_ALGO_SWITCH(algo,
                        if (inverse) return internal_generic::ngfft_execute(plan, calg, ctrue, inout, inout);
                        else return internal_generic::ngfft_execute(plan, calg, cfalse, inout, inout););
}

/**
 * @brief Executes the FFT with separate input and output buffers, runtime direction and algorithm.
 * @param plan The initialized plan.
 * @param inverse If true, performs the inverse FFT; otherwise the forward FFT.
 * @param out Output buffer of `2^plan.l2fftsize` complex elements.
 * @param in Input buffer of `2^plan.l2fftsize` complex elements.
 * @param algo FFT algorithm (defaults to `fourstep`).
 */
template <typename T>
KFR_INLINE void ngfft_execute(const ngfft_plan<T>& plan, bool inverse, complex<T>* out, const complex<T>* in,
                              dft_algorithm algo = default_dft_algorithm)
{
    KFR_DFT_ALGO_SWITCH(algo, if (inverse) return internal_generic::ngfft_execute(plan, calg, ctrue, out, in);
                        else return internal_generic::ngfft_execute(plan, calg, cfalse, out, in););
}

/**
 * @brief Executes the forward real-to-complex FFT with a runtime algorithm.
 * @param plan The initialized plan.
 * @param out Output complex buffer of `2^plan.l2fftsize` complex elements.
 * @param in Input real buffer of `2^(plan.l2fftsize+1)` real elements.
 * @param algo FFT algorithm (defaults to `fourstep`).
 */
template <typename T>
KFR_INLINE void ngfft_real_execute(const ngfft_plan<T>& plan, complex<T>* out, const T* in,
                                   dft_algorithm algo = default_dft_algorithm)
{
    KFR_DFT_ALGO_SWITCH(algo, return internal_generic::ngfft_real_execute(plan, calg, out, in););
}

/**
 * @brief Executes the inverse complex-to-real FFT with a runtime algorithm.
 * @param plan The initialized plan.
 * @param out Output real buffer of `2^(plan.l2fftsize+1)` real elements.
 * @param in Input complex buffer of `2^plan.l2fftsize` complex elements.
 * @param algo FFT algorithm (defaults to `fourstep`).
 */
template <typename T>
KFR_INLINE void ngfft_real_execute(const ngfft_plan<T>& plan, T* out, const complex<T>* in,
                                   dft_algorithm algo = default_dft_algorithm)
{
    KFR_DFT_ALGO_SWITCH(algo, return internal_generic::ngfft_real_execute(plan, calg, out, in););
}

#ifdef KFR_CLASSIC_FFT
/// @brief When true, the classic `dft_plan` uses the ngFFT algorithm for power-of-two sizes.
extern bool fft_ng;
/// @brief When true, the classic `dft_plan` prefers the autosort (no bit-reversal) FFT stages.
extern bool fft_autosort;
/// @brief Algorithm used by the ngFFT path of the classic `dft_plan`.
extern dft_algorithm fft_ng_algorithm;
#endif

namespace internal_generic
{

constexpr inline dft_family to_family(dft_algorithm algo) noexcept { return dft_family::fourstep; }

} // namespace internal_generic

} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)

KFR_PRAGMA_MSVC(warning(pop))
