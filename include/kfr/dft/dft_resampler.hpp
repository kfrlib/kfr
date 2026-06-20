/** @addtogroup convolution
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

#include <span>
#include <bit>

#include "../base/filter.hpp"
#include "../base/memory.hpp"
#include "../simd/complex.hpp"
#include "../simd/constants.hpp"
#include "../simd/read_write.hpp"
#include "../simd/vec.hpp"
#include "../dsp/window.hpp"

#include "../except.hpp"
#include "../test/assert.hpp"

#include "cache.hpp"
#include "fft.hpp"

namespace kfr
{

/**
 * @brief Parameters for constructing a @ref dft_resampler.
 *
 * The resampling factor is restricted to a power of two (positive for
 * upsampling, negative for downsampling, zero for filter-only). The lowpass
 * filter is designed with a Kaiser window from the desired stopband
 * attenuation and transition width.
 */
struct dft_resampler_params
{
    int shift; ///< Resampling factor as a power of 2 (e.g. 1 for 2x, -1 for 0.5x, 0 for filter-only).
    fbase cutoff; ///< Normalised cutoff frequency (0..1], where 1.0 = Nyquist of the output rate.
    fbase stopband_atten_db; ///< Desired stopband attenuation in dB (e.g. 144). Controls Kaiser beta.
    fbase transition_width; ///< Transition bandwidth normalised to the lower-rate Nyquist (0..1].

    size_t
        input_block_size; ///< Computed FFT block size (power of two) satisfying the overlap-save constraint.

    /// @brief Returns the absolute resampling factor (2^|shift|).
    size_t factor() const noexcept { return size_t(1) << std::abs(shift); }
    /// @brief Returns `true` if this is a downsampling configuration.
    bool is_downsampling() const noexcept { return shift < 0; }
    /// @brief Returns the per-stage conversion ratio (>1 for upsampling, <1 for downsampling).
    double stage_factor() const noexcept { return is_downsampling() ? 1.0 / factor() : double(factor()); }

    /**
     * @brief Constructs resampler parameters.
     * @param shift Resampling factor as a power of 2 (positive=up, negative=down, 0=filter-only).
     * @param cutoff Normalised cutoff frequency (0..1] relative to the output Nyquist.
     * @param stopband_atten_db Desired stopband attenuation in dB.
     * @param transition_width Transition bandwidth normalised to the lower-rate Nyquist (0..1).
     * @note `input_block_size` is computed from the Kaiser filter-length formula and
     *       rounded up to the next power of two.
     */
    dft_resampler_params(int shift, fbase cutoff = 1, fbase stopband_atten_db = 144,
                         fbase transition_width = 0.02) noexcept
        : shift(shift), cutoff(cutoff), stopband_atten_db(stopband_atten_db),
          transition_width(transition_width)
    {
        // Validate parameters
        KFR_ASSERT(cutoff > 0);
        KFR_ASSERT(cutoff <= 1);
        KFR_ASSERT(stopband_atten_db >= 0);
        KFR_ASSERT(transition_width > 0);
        KFR_ASSERT(transition_width < 1);

        fbase tw_rad = c_pi<fbase> * transition_width / factor();
        size_t filter_len =
            static_cast<size_t>(std::ceil((stopband_atten_db - fbase(7.95)) / (fbase(2.285) * tw_rad))) + 1;
        filter_len |= 1; // Make odd for exact symmetry
        // Overlap-save constraint: filter_len <= block_size + 1

        input_block_size = std::bit_ceil(std::max<size_t>(filter_len - 1, 256));
    }
};

/**
 * @brief FFT-based sample-rate converter and FIR filter using overlap-save.
 *
 * Performs power-of-two resampling (upsampling, downsampling, or filter-only
 * when the factor is 1) of a real-valued signal. The lowpass anti-aliasing /
 * anti-imaging filter is a Kaiser-windowed sinc designed from the requested
 * stopband attenuation and transition width, pre-transformed into the
 * frequency domain so that each frame only requires a forward real FFT,
 * spectral shaping (gain multiplication and stopband zeroing, plus conjugate
 * mirroring / truncation for up/downsampling), and an inverse real FFT.
 *
 * Processing is block-based (overlap-save): each frame consumes `input_hop()`
 * new input samples and produces `output_hop()` valid output samples, with the
 * circular-convolution artefacts discarded. Use `process()` for arbitrary-length
 * streams, or `process_frame()` for explicit per-frame control.
 *
 * @tparam T Sample type (`float` or `double`).
 */
template <typename T>
class dft_resampler
{
private:
    // Dimensions
    size_t m_input_block_size; // N, Input FFT size
    size_t m_factor; // Resampling factor (power of 2, >= 1)
    bool m_is_downsampling; // true = downsample by m_factor, false = upsample (or filter when factor==1)
    size_t m_output_block_size; // Output FFT size: F*N (up), N/D (down), N (filter-only)
    size_t m_input_hop; // New input samples consumed per frame
    size_t m_output_hop; // Valid output samples produced per frame
    size_t m_discard; // Output samples to discard (circular convolution artefacts)
    size_t m_filter_len; // Actual filter length (in filter-rate samples)

    // Processing buffers (overlap-save)
    univector<T> m_input_buffer; // Sliding input window [N]
    univector<T> m_fft_buffer; // FFT workspace [max(N, M)+2]

    size_t m_input_accum_fill; // How many samples of the current hop have been filled

    univector<T> m_freq_gain; // Pre-computed frequency-domain gain (filter_fft_size/2+1 real values)
    size_t m_transition_start; // Index of first non-1 bin in m_freq_gain (passband end)
    size_t m_transition_stop; // Index of first 0 bin in m_freq_gain (stopband start)

    dft_plan_real<T> m_fft_forward; // Real FFT plan for forward transform (size N)
    dft_plan_real<T> m_fft_inverse; // Real FFT plan for inverse transform (size M = output_block_size)
    univector<uint8_t> m_fft_temp;

public:
    /**
     * @brief Computes the Kaiser window beta from a desired stopband attenuation.
     * @param att Desired stopband attenuation in dB.
     * @return Kaiser beta parameter.
     */
    static T kaiser_beta_from_attenuation(T att)
    {
        if (att > 50)
            return T(0.1102) * (att - T(8.7));
        if (att >= 21)
            return T(0.5842) * pow(att - 21, T(0.4)) + T(0.07886) * (att - 21);
        return 0;
    }

    /// @brief Returns the input FFT block size $N$.
    size_t input_block_size() const noexcept { return m_input_block_size; }
    /// @brief Returns the resampling factor (a power of two, >= 1).
    size_t factor() const noexcept { return m_factor; }
    /// @brief Returns `true` if this resampler downsamples by `factor()`.
    bool is_downsampling() const noexcept { return m_is_downsampling; }
    /// @brief Returns the designed filter length (in filter-rate samples).
    size_t filter_length() const noexcept { return m_filter_len; }
    /// @brief Returns the output FFT block size $M$.
    size_t output_block_size() const noexcept { return m_output_block_size; }
    /// @brief Returns the number of new input samples consumed per frame.
    size_t input_hop() const noexcept { return m_input_hop; }
    /// @brief Returns the number of valid output samples produced per frame.
    size_t output_hop() const noexcept { return m_output_hop; }

    /**
     * @brief Resets the resampler to its freshly-constructed state,
     *        clearing all buffered input without changing filter parameters.
     */
    void reset() noexcept
    {
        m_input_buffer     = zeros<T>();
        m_fft_buffer       = zeros<T>();
        m_input_accum_fill = 0;
    }

    /**
     * @brief Computes the minimal power-of-two block size required for the given
     *        filter parameters (Kaiser formula).
     * @param upsample_factor The upsampling factor used in the transition-width scaling.
     * @param stopband_atten_db Desired stopband attenuation in dB.
     * @param transition_width Transition bandwidth normalised to the lower-rate Nyquist (0..1).
     * @return The computed block size (a power of two).
     */
    static size_t compute_block_size(size_t upsample_factor, T stopband_atten_db, T transition_width)
    {
        KFR_ASSERT(transition_width > T(0));
        KFR_ASSERT(transition_width < T(1));
        T tw_rad = c_pi<T> * transition_width / T(upsample_factor);
        size_t filter_len =
            static_cast<size_t>(std::ceil((stopband_atten_db - T(7.95)) / (T(2.285) * tw_rad))) + 1;
        filter_len |= 1; // Make odd for exact symmetry
        // Overlap-save constraint: filter_len <= block_size + 1
        size_t block_size = std::bit_ceil(std::max<size_t>(filter_len - 1, 256));
        return block_size;
    }

    /**
     * @brief Constructs the resampler from the given parameters.
     *
     * Designs the Kaiser-windowed lowpass filter, transforms it into the
     * frequency domain, and pre-computes the spectral gain and transition-band
     * indices. The forward FFT plan has size `input_block_size` and the inverse
     * FFT plan has size `output_block_size`.
     * @param params Resampler parameters.
     */
    explicit dft_resampler(const dft_resampler_params& params)
        : m_input_block_size(params.input_block_size), m_factor(params.factor()),
          m_is_downsampling(params.is_downsampling()),
          m_output_block_size(params.is_downsampling() ? params.input_block_size / params.factor()
                                                       : params.factor() * params.input_block_size),
          m_fft_forward(params.input_block_size, dft_pack_format::CCs),
          m_fft_inverse(params.is_downsampling() ? params.input_block_size / params.factor()
                                                 : params.factor() * params.input_block_size,
                        dft_pack_format::CCs),
          m_fft_temp(std::max(m_fft_forward.temp_size, m_fft_inverse.temp_size))
    {
        KFR_ASSERT((!m_is_downsampling || (m_input_block_size % m_factor == 0)));

        // The filter is designed at the "larger" rate:
        //   upsampling/filter-only: output rate (F*N, or N when F==1)
        //   downsampling:           input rate  (N)
        const size_t filter_fft_size = m_is_downsampling ? m_input_block_size : m_output_block_size;

        // Cutoff in radians/sample at the filter-design rate.
        // cutoff is normalised to the *output* Nyquist (1.0 = output Nyquist).
        // Shift omega_c down by half the transition width so that the stopband
        // begins exactly at the specified cutoff frequency.
        T omega_c = (m_is_downsampling ? c_pi<T> * params.cutoff / T(m_factor) : c_pi<T> * params.cutoff) -
                    c_pi<T> * params.transition_width / (T(2) * T(m_factor));
        const T beta = kaiser_beta_from_attenuation(params.stopband_atten_db);

        // Filter length from Kaiser formula:
        //   L = ceil((A - 7.95) / (2.285 * delta_omega)) + 1
        //   delta_omega = pi * tw / factor (at the filter rate)
        size_t filter_len;
        if (params.transition_width > T(0))
        {
            T tw_rad = c_pi<T> * params.transition_width / T(m_factor);
            filter_len =
                static_cast<size_t>(std::ceil((params.stopband_atten_db - T(7.95)) / (T(2.285) * tw_rad))) +
                1;
        }
        else
        {
            filter_len = m_input_block_size + 1; // Max length → narrowest transition
        }
        // Clamp to N+1: practical limit for overlap-save efficiency.
        filter_len = std::min(filter_len, m_input_block_size + 1);
        filter_len |= 1; // Make odd for exact symmetry
        m_filter_len = filter_len;

        const size_t half_len = filter_len / 2;

        // Build windowed sinc lowpass (time domain)
        univector<T> h_filter(filter_len);
        h_filter = window_kaiser<T>(filter_len, beta);
        for (size_t i = 0; i < filter_len; ++i)
        {
            T n = T(i) - T(half_len);
            h_filter[i] *= sinc(omega_c * n); // kaiser[i] * sin(wc*n)/(wc*n)
        }

        // Circular-shift into a zero-phase buffer so the filter center is at sample 0
        // (makes the frequency response real-valued / zero-phase)
        univector<T> h_padded(filter_fft_size, T(0));
        for (size_t i = 0; i < filter_len; ++i)
        {
            int n      = static_cast<int>(i) - static_cast<int>(half_len);
            size_t idx = static_cast<size_t>(
                (n % static_cast<int>(filter_fft_size) + static_cast<int>(filter_fft_size)) %
                static_cast<int>(filter_fft_size));
            h_padded[idx] = h_filter[i];
        }

        // Forward FFT (size filter_fft_size) to get the frequency response
        univector<T> h_fft_buf(filter_fft_size + 2, T(0));
        auto* H = reinterpret_cast<complex<T>*>(h_fft_buf.data());
        if (m_is_downsampling)
            m_fft_forward.execute(H, h_padded.data(), m_fft_temp.data());
        else
            m_fft_inverse.execute(H, h_padded.data(), m_fft_temp.data());

        const size_t num_gain_bins = filter_fft_size / 2 + 1;
        // Normalise so passband gain = 1
        const T dc_gain = H[0].real();
        m_freq_gain.resize(num_gain_bins);
        for (size_t k = 0; k < num_gain_bins; ++k)
        {
            m_freq_gain[k] = H[k].real() / dc_gain;
        }

        constexpr T epsilon = T(1e-6);

        m_transition_start =
            std::find_if(m_freq_gain.begin(), m_freq_gain.end(), [](T v) { return v < (1.0 - epsilon); }) -
            m_freq_gain.begin();

        m_transition_stop = num_gain_bins - (std::find_if(m_freq_gain.rbegin(), m_freq_gain.rend(),
                                                          [](T v) { return v > (0.0 + epsilon); }) -
                                             m_freq_gain.rbegin());

        // Overlap-save dimensions derived from filter length
        m_discard = filter_len - 1;
        if (m_is_downsampling)
        {
            // discard starts in input samples; align to 2*factor for clean output division
            m_discard    = align_up(m_discard, size_t(2) * m_factor);
            m_input_hop  = m_input_block_size - m_discard;
            m_discard    = m_discard / m_factor; // convert to output samples
            m_output_hop = m_input_hop / m_factor;
        }
        else
        {
            // discard is in output samples; align to factor for clean input division
            m_discard    = align_up(m_discard, m_factor);
            m_output_hop = m_output_block_size - m_discard;
            m_input_hop  = m_output_hop / m_factor;
        }

        // Pre-allocate processing buffers
        m_input_buffer.resize(m_input_block_size);
        m_fft_buffer.resize(std::max(m_input_block_size, m_output_block_size) + 2);
        reset();
    }

private:
    static void mirror(complex<T>* spectrum, size_t input_block_size, size_t factor)
    {
        const size_t num_input_bins = input_block_size / 2 + 1;
        const size_t num_stages     = std::countr_zero(factor); // log2(factor), power-of-2 guaranteed
        if (num_stages == 0)
            return; // factor==1: no upsampling, forward and inverse FFTs are the same size

        size_t n = num_input_bins - 1; // current Nyquist index

        // Stage 0: conjugate mirror — imaginary parts must be negated, so no memcpy possible
        make_univector(spectrum + n + 1, n) = cconj(reverse(make_univector(spectrum, n)));
        n *= 2;

        // Stages 1+: the stage-0 output is already Hermitian symmetric, so:
        //   conj(Y[n - k])  ==  conj(conj(Y[k]))  ==  Y[k]
        // Mirror degenerates into a plain copy — memcpy the whole block
        for (size_t stage = 1; stage < num_stages; ++stage)
        {
            make_univector(spectrum + n, n) = make_univector(spectrum, n);
            spectrum[2 * n]                 = std::complex<T>(spectrum[0].real(), T{}); // new Nyquist
            n *= 2;
        }
    }

public:
    /**
     * @brief Processes one frame in-place and returns a pointer to the valid output.
     *
     * Performs the forward real FFT of the `input_block_size()` samples at
     * `input`, applies spectral shaping (mirroring/truncation, transition-band
     * gain, stopband zeroing), and the inverse real FFT. The returned pointer
     * references `output_hop()` valid samples inside the resampler's internal
     * `m_fft_buffer`; the data is **not** normalised (caller must scale by
     * `1/input_block_size()` if needed). The pointer is valid only until the
     * next call to any processing function.
     * @param input Pointer to `input_block_size()` input samples.
     * @return Pointer to `output_hop()` valid (un-normalised) output samples.
     */
    T* process_frame(const T* input)
    {
        // --- 1. Forward Real FFT (size N) of the sliding input buffer ---
        auto* spectrum = reinterpret_cast<complex<T>*>(m_fft_buffer.data());
        m_fft_forward.execute(spectrum, input, m_fft_temp.data());

        // --- 2. Spectral Shaping ---
        if (!m_is_downsampling)
        {
            // Upsampling / filter-only path
            const size_t num_output_bins = m_output_block_size / 2 + 1;
            auto spectrum_ref            = make_univector(spectrum, num_output_bins);

            // Mirror input spectrum for spectral zero-insertion (no-op when factor==1)
            mirror(spectrum, m_input_block_size, m_factor);

            // Apply lowpass in transition band
            spectrum_ref.slice(m_transition_start, num_output_bins - m_transition_stop) *=
                m_freq_gain.slice(m_transition_start, num_output_bins - m_transition_stop);

            // Zero stopband
            spectrum_ref.slice(m_transition_stop, num_output_bins - m_transition_stop) = zeros<complex<T>>();
        }
        else
        {
            // Downsampling path: shape at input rate, truncate for smaller inverse FFT
            const size_t num_gain_bins   = m_freq_gain.size(); // N/2+1
            const size_t num_output_bins = m_output_block_size / 2 + 1; // M/2+1
            auto spectrum_ref            = make_univector(spectrum, num_gain_bins);

            // Apply lowpass in transition band (operates on full input spectrum)
            spectrum_ref.slice(m_transition_start, num_gain_bins - m_transition_stop) *=
                m_freq_gain.slice(m_transition_start, num_gain_bins - m_transition_stop);

            // Zero stopband in input spectrum
            spectrum_ref.slice(m_transition_stop, num_gain_bins - m_transition_stop) = zeros<complex<T>>();

            // Force new Nyquist bin to be real for the smaller inverse FFT
            if (m_factor > 1)
                spectrum[num_output_bins - 1].imag(T(0));
        }

        // --- 3. Inverse Real FFT (size M = output_block_size) ---
        m_fft_inverse.execute(m_fft_buffer.data(), spectrum, m_fft_temp.data());

        // --- 4. Discard corrupted samples (circular convolution artefacts),
        //        copy valid output with normalisation ---
        // The filter is stored in zero-phase form (centred at sample 0), so
        // corruption is split equally at both ends.  The valid region starts at m_discard/2.
        const size_t valid_start = m_discard / 2;
        return m_fft_buffer.data() + valid_start;
    }
    /**
     * @brief Processes one frame via overlap-save and writes normalised output.
     *
     * Equivalent to the pointer overload of @ref process_frame, but copies the
     * `output_hop()` valid samples into `output` and applies the `1/N`
     * normalisation so the result has unit passband gain.
     * @param output Output buffer of at least `output_hop()` samples.
     * @param input Pointer to `input_block_size()` input samples.
     */
    void process_frame(T* output, const T* input)
    {
        const T scale                        = T(1) / m_input_block_size;
        T* valid_output_start                = process_frame(input);
        make_univector(output, m_output_hop) = make_univector(valid_output_start, m_output_hop) * scale;
    }

    /**
     * @brief Processes an arbitrary-length input stream and writes the resampled output.
     *
     * Internally accumulates input into `input_hop()`-sized hops and calls
     * `process_frame()` for each complete hop. A fast path is taken when the
     * overlap region and at least one full hop are available contiguously in
     * the input span (avoiding internal copies); otherwise samples are copied
     * into the sliding input buffer.
     * @param output Output buffer. Must be large enough to hold all frames
     *        produced (a logic check is performed per frame).
     * @param input Input samples (any size).
     * @return Number of output samples actually written.
     */
    size_t process(std::span<T> output, std::span<const T> input)
    {
        size_t in_pos        = 0;
        size_t out_pos       = 0;
        const size_t overlap = m_input_block_size - m_input_hop;

        while (in_pos < input.size())
        {
            // 1. When starting a new hop, try the fast path first
            if (m_input_accum_fill == 0)
            {
                // Fast path: when the overlap (tail of the previous block) and at least
                // one full hop are both available contiguously in the input span, call
                // process_frame directly on the input pointer — no memmove, no memcpy.
                if (in_pos >= overlap && input.size() - in_pos >= m_input_hop)
                {
                    do
                    {
                        KFR_LOGIC_CHECK(output.size() - out_pos >= m_output_hop, "Output buffer too small");
                        process_frame(output.data() + out_pos, input.data() + in_pos - overlap);
                        out_pos += m_output_hop;
                        in_pos += m_input_hop;
                    } while (input.size() - in_pos >= m_input_hop);

                    // Restore m_input_buffer with the last processed block so that
                    // subsequent slow-path frames or the next process() call see
                    // correct overlap data.  in_pos >= m_input_block_size is
                    // guaranteed because we entered with in_pos >= overlap and
                    // consumed at least one hop (overlap + hop == block_size).
                    builtin_memcpy(m_input_buffer.data(), input.data() + in_pos - m_input_block_size,
                                   m_input_block_size * sizeof(T));

                    if (in_pos >= input.size())
                        break;
                }

                // Slow path: slide overlap from the previous block to front of buffer
                std::memmove(m_input_buffer.data(), m_input_buffer.data() + m_input_hop, overlap * sizeof(T));
            }

            // 2. Fill directly into the tail of m_input_buffer
            {
                const size_t tail_offset = m_input_block_size - m_input_hop + m_input_accum_fill;
                size_t to_copy           = std::min(input.size() - in_pos, m_input_hop - m_input_accum_fill);
                builtin_memcpy(m_input_buffer.data() + tail_offset, input.data() + in_pos,
                               to_copy * sizeof(T));
                m_input_accum_fill += to_copy;
                in_pos += to_copy;
            }

            // 3. When a full hop is accumulated, process frame
            if (m_input_accum_fill == m_input_hop)
            {
                KFR_LOGIC_CHECK(output.size() - out_pos >= m_output_hop, "Output buffer too small");
                process_frame(output.data() + out_pos, m_input_buffer.data());
                out_pos += m_output_hop;
                m_input_accum_fill = 0;
            }
        }

        return out_pos;
    }
};

} // namespace kfr
