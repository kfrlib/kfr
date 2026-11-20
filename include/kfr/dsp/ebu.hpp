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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

#include "../base.hpp"
#include "../test/assert.hpp"
#include "biquad.hpp"
#include "biquad_design.hpp"
#include "speaker.hpp"
#include "units.hpp"

KFR_PRAGMA_GNU(GCC diagnostic push)
#if KFR_HAS_WARNING("-Winaccessible-base")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Winaccessible-base")
#endif

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Convert mean-square energy to loudness in LUFS.
 *
 * Implements the EBU R128 mapping @f$ L = 10 \cdot \log_{10}(z) - 0.691 @f$,
 * where @c z is the K-weighted mean-square energy.
 * @param energy K-weighted mean-square energy (linear, not dB).
 * @return Loudness in LUFS.
 */
template <typename T>
KFR_INTRINSIC T energy_to_loudness(T energy)
{
    return T(10) * log10(energy) - T(0.691);
}

/**
 * @brief Inverse of @ref energy_to_loudness: convert LUFS back to linear energy.
 *
 * @param loudness Loudness in LUFS.
 * @return K-weighted mean-square energy (linear).
 */
template <typename T>
KFR_INTRINSIC T loudness_to_energy(T loudness)
{
    return exp10((loudness + T(0.691)) * T(0.1));
}

/**
 * @brief Fixed-memory histogram of absolutely gated mean-square energies.
 *
 * 10,000 regular bins span -70 to +30 LUFS at 0.01 LU resolution, plus a
 * final bin for +30 LUFS and above. Such high levels are not expected in a
 * normal audio workflow. Gating includes the bin containing the gate; gated
 * averages and percentile positions use the bin's lower-edge loudness. The
 * ungated mean uses the original energies.
 */
template <typename T>
class loudness_histogram
{
    static constexpr size_t regular_bins = 10000;

    static size_t bin_index(double loudness)
    {
        if (loudness <= -70.0)
            return 0;
        if (loudness >= 30.0)
            return regular_bins;
        return static_cast<size_t>((loudness + 70.0) * 100.0);
    }

    static double bin_loudness(size_t index)
    {
        return -70.0 + static_cast<double>(index) * 0.01;
    }

public:
    void push(T mean_square)
    {
        const double energy = static_cast<double>(mean_square);
        if (!std::isfinite(energy) || energy <= 0)
            return;
        const double loudness = energy_to_loudness(energy);
        if (loudness < -70.0)
            return;
        ++bins[bin_index(loudness)];
        total_energy += energy;
        ++total_count;
    }

    void reset()
    {
        bins.fill(0);
        total_energy = 0;
        total_count = 0;
    }

    T integrated() const
    {
        if (total_count == 0)
            return -c_infinity<T>;
        const double threshold = energy_to_loudness(total_energy / static_cast<double>(total_count)) - 10.0;
        double energy = 0;
        uint64_t count = 0;
        for (size_t i = bin_index(threshold); i < bins.size(); ++i)
        {
            if (bins[i] != 0)
            {
                energy += loudness_to_energy(bin_loudness(i)) * static_cast<double>(bins[i]);
                count += bins[i];
            }
        }
        return count ? static_cast<T>(energy_to_loudness(energy / static_cast<double>(count))) : -c_infinity<T>;
    }

    void range(T& low, T& high) const
    {
        low = high = T(-70);
        if (total_count == 0)
            return;
        const double threshold = energy_to_loudness(total_energy / static_cast<double>(total_count)) - 20.0;
        const size_t first_bin = bin_index(threshold);
        uint64_t count = 0;
        for (size_t i = first_bin; i < bins.size(); ++i)
            count += bins[i];
        if (count == 0)
            return;
        const uint64_t low_rank = static_cast<uint64_t>(std::round((count - 1) * 0.10));
        const uint64_t high_rank = static_cast<uint64_t>(std::round((count - 1) * 0.95));
        uint64_t seen = 0;
        bool found_low = false;
        for (size_t i = first_bin; i < bins.size(); ++i)
        {
            if (!bins[i])
                continue;
            const uint64_t end = seen + bins[i];
            const T value = static_cast<T>(bin_loudness(i));
            if (!found_low && low_rank < end)
            {
                low = value;
                found_low = true;
            }
            if (high_rank < end)
            {
                high = value;
                return;
            }
            seen = end;
        }
    }

private:
    std::array<uint64_t, regular_bins + 1> bins{};
    double total_energy = 0;
    uint64_t total_count = 0;
};

/** @brief Fixed-memory integrated loudness accumulator; gated values are approximated by histogram bins. */
template <typename T>
struct integrated_vec
{
    void push(T mean_square) { histogram.push(mean_square); cached = false; }
    void reset() { histogram.reset(); cached = false; }
    T get() const
    {
        if (!cached)
        {
            value = histogram.integrated();
            cached = true;
        }
        return value;
    }

private:
    loudness_histogram<T> histogram;
    mutable T value = -c_infinity<T>;
    mutable bool cached = false;
};

/** @brief Fixed-memory short-term loudness histogram for LRA percentiles. */
template <typename T>
struct lra_vec
{
    void push(T mean_square) { histogram.push(mean_square); cached = false; }
    void reset() { histogram.reset(); cached = false; }
    void get(T& low, T& high) const
    {
        if (!cached)
        {
            histogram.range(low_value, high_value);
            cached = true;
        }
        low = low_value;
        high = high_value;
    }

private:
    loudness_histogram<T> histogram;
    mutable T low_value = T(-70), high_value = T(-70);
    mutable bool cached = false;
};

/**
 * @brief Build the coefficients for the EBU R128 K-weighting pre-filter.
 *
 * The K-filter is a +4 dB high-shelf at 1681.81 Hz followed by a high-pass at
 * 38.1106678 Hz (Q = 0.5, normalized). It must be applied to each channel
 * before measuring energy.
 * @param samplerate Sample rate in Hz.
 * @return Coefficients for the two-section K-weighting filter.
 */
template <typename T>
KFR_INTRINSIC iir_params<T, 2> make_kfilter_params(int samplerate)
{
    // BS.1770 K-weighting parameters, transformed to the requested sample rate.
    const double k = std::tan(c_pi<double> * 1681.974450955533 / samplerate);
    const double q = 0.7071752369554196;
    const double vh = std::pow(10.0, 3.999843853973347 / 20.0);
    const double vb = std::pow(vh, 0.4996667741545416);
    const double denom = 1.0 + k / q + k * k;
    const biquad_section<T> shelf{
        T(1), T(2 * (k * k - 1) / denom), T((1 - k / q + k * k) / denom),
        T((vh + vb * k / q + k * k) / denom), T(2 * (k * k - vh) / denom),
        T((vh - vb * k / q + k * k) / denom)
    };
    auto highpass = biquad_highpass<T>(T(38.13547087602444 / samplerate), T(0.5003270373238773));
    highpass.b0 = T(1);
    highpass.b1 = T(-2);
    highpass.b2 = T(1);
    const biquad_section<T> sections[] = { shelf, highpass };
    return iir_params{ sections };
}

template <typename T>
struct ebu_r128;

/**
 * @brief Single-channel loudness measurement front-end.
 *
 * Applies the K-weighting filter to each packet, then accumulates the
 * filtered sum-of-squares into two ring buffers:
 *  - @c m_momentary_sum_of_squares covers 400 ms (momentary loudness, M).
 *  - @c m_short_sum_of_squares covers 3 s (short-term loudness, S).
 *
 * Per-channel output gain accounts for the EBU channel weighting
 * (e.g. +1.5 dB for surrounds, 0 dB for LFE).
 */
template <typename T>
struct ebu_channel
{
public:
    friend struct ebu_r128<T>;
    /**
     * @brief Construct a channel front-end.
     * @param sample_rate         Sample rate in Hz.
     * @param speaker             Speaker role (determines output weighting).
     * @param packet_size_factor  1 = 10 Hz refresh, 2 = 20 Hz, 3 = 30 Hz.
     * @param input_gain          Linear gain applied to the input before filtering.
     */
    ebu_channel(int sample_rate, speaker_type speaker, int packet_size_factor = 1, T input_gain = 1)
        : m_sample_rate(sample_rate), m_speaker(speaker), m_input_gain(input_gain),
          m_packet_size(sample_rate / 10 / packet_size_factor), m_kfilter(make_kfilter_params<T>(sample_rate)),
          m_short_sum_of_squares(3000 / 100 * packet_size_factor),
          m_momentary_sum_of_squares(400 / 100 * packet_size_factor), m_output_energy_gain(1.0),
          m_buffer_cursor(0), m_short_sum_of_squares_cursor(0), m_momentary_sum_of_squares_cursor(0)
    {
        switch (speaker)
        {
        case speaker_type::Lfe:
        case speaker_type::Lfe2:
            m_output_energy_gain = 0.0;
            break;
        case speaker_type::LeftSurround:
        case speaker_type::RightSurround:
            m_output_energy_gain = static_cast<T>(dB_to_power(+1.5));
            break;
        default:
            break;
        }
        reset();
    }

    /** @brief Reset the K-filter and the momentary and short-term energy histories. */
    void reset()
    {
        m_kfilter.state = {};
        m_kfilter.saved_state = {};
        m_kfilter.block_end = 0;
        std::fill(m_short_sum_of_squares.begin(), m_short_sum_of_squares.end(), T(0));
        std::fill(m_momentary_sum_of_squares.begin(), m_momentary_sum_of_squares.end(), T(0));
        m_short_sum_of_squares_cursor = 0;
        m_momentary_sum_of_squares_cursor = 0;
    }

    /**
     * @brief Process one packet of @ref packet_size() samples.
     *
     * Filters the input through the K-filter and adds the resulting
     * sum-of-squares to both ring buffers (overwriting the oldest slot).
     * @param src Pointer to @ref packet_size() input samples.
     */
    void process_packet(const T* src)
    {
        const auto input = make_univector(src, m_packet_size) * m_input_gain;
        process_input(input);
    }
    /** @brief Return the speaker role assigned to this channel. */
    speaker_type get_speaker() const { return m_speaker; }
    /** @brief Return the number of samples per packet. */
    size_t packet_size() const { return m_packet_size; }

private:
    template <typename Input>
    void process_input(const Input& input)
    {
        const T energy = sumsqr(iir(input, std::ref(m_kfilter)));
        m_short_sum_of_squares.ringbuf_write(m_short_sum_of_squares_cursor, energy);
        m_momentary_sum_of_squares.ringbuf_write(m_momentary_sum_of_squares_cursor, energy);
    }

    void process_silence()
    {
        process_input(truncate(counter<T>(T(0), T(0)), m_packet_size));
    }

    const int m_sample_rate;
    const speaker_type m_speaker;
    const T m_input_gain;
    const size_t m_packet_size;
    iir_state<T, 2> m_kfilter;
    univector<T> m_short_sum_of_squares;
    univector<T> m_momentary_sum_of_squares;
    T m_output_energy_gain;
    univector<T> m_buffer;
    size_t m_buffer_cursor;
    size_t m_short_sum_of_squares_cursor;
    size_t m_momentary_sum_of_squares_cursor;
};

/**
 * @brief EBU R128 loudness meter.
 *
 * Computes momentary (M, 400 ms), short-term (S, 3 s), integrated (I) and
 * loudness range (LRA) values for a multi-channel signal, per
 * EBU Tech 3341 / R128 s2 and EBU R128 s3 (LRA).
 *
 * Packets are @c sample_rate/10/ @c packet_size_factor samples long, i.e. the
 * measurement refresh rate is @c 10 * packet_size_factor Hz. Packet processing
 * uses fixed-size histograms and does not allocate. Integrated and LRA results
 * use 0.01 LU bins and include the entire bin containing the relative gate;
 * errors can be larger near a populated gate. Loudness above +30 LUFS, not
 * expected in normal audio workflows, is clamped; non-finite energies are
 * ignored. Reading an uncached integrated
 * or LRA value scans the histograms.
 */
template <typename T>
struct ebu_r128
{
public:
    /**
     * @brief Construct the meter.
     * @param sample_rate         Sample rate in Hz.
     * @param channels            Speaker roles for each input channel (L, R, C, Ls, Rs, LFE, ...).
     * @param packet_size_factor  Refresh multiplier: 1 = 10 Hz, 2 = 20 Hz, 3 = 30 Hz (range [1..6]).
     *
     * One @ref ebu_channel is created per entry in @c channels, in order.
     */
    ebu_r128(int sample_rate, std::span<const speaker_type> channels, int packet_size_factor = 1)
        : m_sample_rate(sample_rate), m_running(true), m_need_reset(false),
          m_packet_size(sample_rate / 10 / packet_size_factor), m_packet_size_factor(packet_size_factor)
    {
        KFR_LOGIC_CHECK(!channels.empty(), "channels must not be empty");
        KFR_LOGIC_CHECK(sample_rate > 0, "sample_rate must be greater than 0");
        KFR_LOGIC_CHECK(packet_size_factor >= 1 && packet_size_factor <= 6,
                        "packet_size_factor must be in range [1..6]");
        for (speaker_type sp : channels)
        {
            m_channels.emplace_back(sample_rate, sp, packet_size_factor, T(1));
        }
    }

    /** @brief Return the configured sample rate in Hz. */
    int sample_rate() const { return m_sample_rate; }

    /** @brief Return the number of samples per packet (= sample_rate / refresh_rate). */
    size_t packet_size() const { return m_packet_size; }

    /**
     * @brief Read all measured loudness values.
     * @param loudness_momentary     Out: momentary loudness M (400 ms), LUFS.
     * @param loudness_short         Out: short-term loudness S (3 s), LUFS.
     * @param loudness_intergrated   Out: integrated program loudness I, LUFS.
     * @param loudness_range_low     Out: LRA 10 % percentile, LUFS.
     * @param loudness_range_high    Out: LRA 95 % percentile, LUFS.
     * @note LRA = @c loudness_range_high - @c loudness_range_low.
     */
    void get_values(T& loudness_momentary, T& loudness_short, T& loudness_intergrated, T& loudness_range_low,
                    T& loudness_range_high)
    {
        T sum_of_mean_square_momentary = 0;
        T sum_of_mean_square_short     = 0;
        for (size_t ch = 0; ch < m_channels.size(); ch++)
        {
            sum_of_mean_square_momentary += mean(m_channels[ch].m_momentary_sum_of_squares) / m_packet_size *
                                            m_channels[ch].m_output_energy_gain;
            sum_of_mean_square_short += mean(m_channels[ch].m_short_sum_of_squares) / m_packet_size *
                                        m_channels[ch].m_output_energy_gain;
        }
        loudness_momentary   = energy_to_loudness(sum_of_mean_square_momentary);
        loudness_short       = energy_to_loudness(sum_of_mean_square_short);
        loudness_intergrated = m_integrated_buffer.get();
        m_lra_buffer.get(loudness_range_low, loudness_range_high);
    }

    /** @brief Access channel @c index (read-only). */
    const ebu_channel<T>& operator[](size_t index) const { return m_channels[index]; }
    /** @brief Return the number of channels. */
    size_t count() const { return m_channels.size(); }

    /**
     * @brief Process one packet for every channel.
     *
     * Each entry of @c source must contain exactly @ref packet_size() samples
     * and correspond, in order, to the channels passed to the constructor.
     * Updates the momentary, short-term, integrated and LRA buffers while
     * @ref running() is true.
     */
    void process_packet(const std::initializer_list<univector_dyn<T>>& source)
    {
        process_packet_impl<tag_dynamic_vector>(source);
    }
    /// @copydoc process_packet(const std::initializer_list<univector_dyn<T>>&)
    void process_packet(const std::initializer_list<univector_ref<T>>& source)
    {
        process_packet_impl<tag_array_ref>(source);
    }
    /// @copydoc process_packet(const std::initializer_list<univector_dyn<T>>&)
    void process_packet(std::span<const univector_dyn<T>> source)
    {
        process_packet_impl<tag_dynamic_vector>(source);
    }
    /// @copydoc process_packet(const std::initializer_list<univector_dyn<T>>&)
    void process_packet(std::span<const univector_ref<T>> source)
    {
        process_packet_impl<tag_array_ref>(source);
    }

    /**
     * @brief Process @p num_packets of silence on every channel.
     *
     * Equivalent to processing zero-filled packets: the K-filter's decay, window
     * histories, gating and deferred reset are preserved. No input buffers are allocated.
     * @param num_packets Number of packets, each @ref packet_size() samples long.
     */
    void process_silence_packets(size_t num_packets)
    {
        for (size_t i = 0; i < num_packets; ++i)
            process_channels([](ebu_channel<T>& channel, size_t) { channel.process_silence(); });
    }

    /** @brief (Re)start measurement: subsequent packets update the buffers. */
    void start() { m_running = true; }
    /** @brief Pause measurement: packets are still filtered but buffers are frozen. */
    void stop() { m_running = false; }
    /** @brief Return whether the meter is currently running (see @ref start(), @ref stop()). */
    bool running() const { return m_running; }
    /**
     * @brief Request a deferred reset of all channels and accumulators.
     *
     * The reset is applied at the start of the next processed packet, including
     * one supplied by @ref process_silence_packets.
     */
    void reset() { m_need_reset = true; }

private:
    /**
     * @brief Per-packet implementation shared by all @ref process_packet overloads.
     *
     * Filters every channel, accumulates momentary/short-term energy, and (when
     * running) pushes those energies into the integrated and LRA buffers. Honors
     * a pending @ref reset() request before accumulating.
     * @param source One packet per channel, in constructor order.
     */
    template <univector_tag Tag>
    void process_packet_impl(std::span<const univector<T, Tag>> source)
    {
        process_channels([&](ebu_channel<T>& channel, size_t ch)
                         {
                             KFR_ASSERT(source[ch].size() == m_packet_size);
                             channel.process_packet(source[ch].data());
                         });
    }

    template <typename ProcessChannel>
    void process_channels(ProcessChannel&& process_channel)
    {
        T momentary = 0;
        T shortterm = 0;
        if (m_need_reset)
        {
            m_need_reset = false;
            for (size_t ch = 0; ch < m_channels.size(); ch++)
            {
                m_channels[ch].reset();
            }
            m_integrated_buffer.reset();
            m_lra_buffer.reset();
            m_packets_seen = 0;
        }
        for (size_t ch = 0; ch < m_channels.size(); ch++)
        {
            ebu_channel<T>& chan = m_channels[ch];
            process_channel(chan, ch);
            if (m_running)
            {
                momentary += mean(m_channels[ch].m_momentary_sum_of_squares) / m_packet_size *
                             m_channels[ch].m_output_energy_gain;
                shortterm += mean(m_channels[ch].m_short_sum_of_squares) / m_packet_size *
                             m_channels[ch].m_output_energy_gain;
            }
        }
        if (m_packets_seen < 30 * static_cast<size_t>(m_packet_size_factor))
            ++m_packets_seen;
        if (m_running)
        {
            // Only complete 400 ms and 3 s windows contribute to the histograms.
            if (m_packets_seen >= 4 * static_cast<size_t>(m_packet_size_factor))
                m_integrated_buffer.push(momentary);
            if (m_packets_seen >= 30 * static_cast<size_t>(m_packet_size_factor))
                m_lra_buffer.push(shortterm);
        }
    }
    int m_sample_rate;
    bool m_running;
    bool m_need_reset;
    size_t m_packet_size;
    int m_packet_size_factor;
    size_t m_packets_seen = 0;
    std::vector<ebu_channel<T>> m_channels;
    integrated_vec<T> m_integrated_buffer;
    lra_vec<T> m_lra_buffer;
};

} // namespace KFR_ARCH_NAME
} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)
