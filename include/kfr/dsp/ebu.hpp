/** @addtogroup ebu
 *  @{
 */
/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
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

#include <vector>

#include "../base.hpp"
#include "../testo/assert.hpp"
#include "biquad.hpp"
#include "biquad_design.hpp"
#include "speaker.hpp"
#include "units.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Winaccessible-base")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Winaccessible-base")
#endif

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T>
KFR_INTRINSIC T energy_to_loudness(T energy)
{
    return T(10) * log10(energy) - T(0.691);
}

template <typename T>
KFR_INTRINSIC T loudness_to_energy(T loudness)
{
    return exp10((loudness + T(0.691)) * T(0.1));
}

template <typename T>
struct integrated_vec : public univector<T>
{
private:
    void compute() const
    {
        const T z_total = mean(static_cast<const univector<T>&>(*this));
        T relative_gate = energy_to_loudness(z_total) - 10;

        T z        = 0;
        size_t num = 0;
        for (T v : *this)
        {
            T lk = energy_to_loudness(v);
            if (lk >= relative_gate)
            {
                z += v;
                num++;
            }
        }
        z /= num;
        if (num >= 1)
        {
            m_integrated = energy_to_loudness(z);
        }
        else
        {
            m_integrated = -c_infinity<T>;
        }

        m_integrated_cached = true;
    }

public:
    integrated_vec() : m_integrated(-c_infinity<T>), m_integrated_cached(false) {}
    void push(T mean_square)
    {
        T lk = energy_to_loudness(mean_square);
        if (lk >= T(-70.))
        {
            this->push_back(mean_square);
            m_integrated_cached = false;
        }
    }
    void reset()
    {
        m_integrated_cached = false;
        this->clear();
    }
    T get() const
    {
        if (!m_integrated_cached)
        {
            compute();
        }
        return m_integrated;
    }

private:
    mutable T m_integrated;
    mutable bool m_integrated_cached;
};

template <typename T>
struct lra_vec : public univector<T>
{
private:
    void compute() const
    {
        m_range_high            = -70;
        m_range_low             = -70;
        static const T PRC_LOW  = T(0.10);
        static const T PRC_HIGH = T(0.95);

        const T z_total       = mean(static_cast<const univector<T>&>(*this));
        const T relative_gate = energy_to_loudness(z_total) - 20;

        if (this->size() < 2)
            return;

        const size_t start_index =
            std::upper_bound(this->begin(), this->end(), loudness_to_energy(relative_gate)) - this->begin();
        if (this->size() - start_index < 2)
            return;
        const size_t end_index = this->size() - 1;

        const size_t low_index =
            static_cast<size_t>(std::llround(start_index + (end_index - start_index) * PRC_LOW));
        const size_t high_index =
            static_cast<size_t>(std::llround(start_index + (end_index - start_index) * PRC_HIGH));
        m_range_low  = energy_to_loudness((*this)[low_index]);
        m_range_high = energy_to_loudness((*this)[high_index]);

        m_lra_cached = true;
    }

public:
    lra_vec() : m_range_low(-70), m_range_high(-70), m_lra_cached(false) {}
    void push(T mean_square)
    {
        const T lk = energy_to_loudness(mean_square);
        if (lk >= -70)
        {
            auto it = std::upper_bound(this->begin(), this->end(), mean_square);
            this->insert(it, mean_square);
            m_lra_cached = false;
        }
    }
    void reset()
    {
        m_lra_cached = false;
        this->clear();
    }
    void get(T& low, T& high) const
    {
        if (!m_lra_cached)
            compute();
        low  = m_range_low;
        high = m_range_high;
    }

private:
    mutable T m_range_low;
    mutable T m_range_high;
    mutable bool m_lra_cached;
};

template <typename T>
KFR_INTRINSIC expression_handle<T, 1> make_kfilter(int samplerate)
{
    const biquad_params<T> bq[] = {
        biquad_highshelf(T(1681.81 / samplerate), T(+4.0)),
        biquad_highpass(T(38.1106678246655 / samplerate), T(0.5)).normalized_all()
    };
    return to_handle(biquad(bq, placeholder<T>()));
}

template <typename T>
struct ebu_r128;

template <typename T>
struct ebu_channel
{
public:
    friend struct ebu_r128<T>;
    ebu_channel(int sample_rate, Speaker speaker, int packet_size_factor = 1, T input_gain = 1)
        : m_sample_rate(sample_rate), m_speaker(speaker), m_input_gain(input_gain),
          m_packet_size(sample_rate / 10 / packet_size_factor), m_kfilter(make_kfilter<T>(sample_rate)),
          m_short_sum_of_squares(3000 / 100 * packet_size_factor),
          m_momentary_sum_of_squares(400 / 100 * packet_size_factor), m_output_energy_gain(1.0),
          m_buffer_cursor(0), m_short_sum_of_squares_cursor(0), m_momentary_sum_of_squares_cursor(0)
    {
        switch (speaker)
        {
        case Speaker::Lfe:
        case Speaker::Lfe2:
            m_output_energy_gain = 0.0;
            break;
        case Speaker::LeftSurround:
        case Speaker::RightSurround:
            m_output_energy_gain = dB_to_power(+1.5);
            break;
        default:
            break;
        }
        reset();
    }

    void reset()
    {
        std::fill(m_short_sum_of_squares.begin(), m_short_sum_of_squares.end(), T(0));
        std::fill(m_momentary_sum_of_squares.begin(), m_momentary_sum_of_squares.end(), T(0));
    }

    void process_packet(const T* src)
    {
        substitute(m_kfilter, to_handle(make_univector(src, m_packet_size) * m_input_gain));
        const T filtered_sum_of_squares = sumsqr(truncate(m_kfilter, m_packet_size));

        m_short_sum_of_squares.ringbuf_write(m_short_sum_of_squares_cursor, filtered_sum_of_squares);
        m_momentary_sum_of_squares.ringbuf_write(m_momentary_sum_of_squares_cursor, filtered_sum_of_squares);
    }
    Speaker get_speaker() const { return m_speaker; }

private:
    const int m_sample_rate;
    const Speaker m_speaker;
    const T m_input_gain;
    const size_t m_packet_size;
    expression_handle<T, 1> m_kfilter;
    univector<T> m_short_sum_of_squares;
    univector<T> m_momentary_sum_of_squares;
    T m_output_energy_gain;
    univector<T> m_buffer;
    size_t m_buffer_cursor;
    size_t m_short_sum_of_squares_cursor;
    size_t m_momentary_sum_of_squares_cursor;
};

template <typename T>
struct ebu_r128
{
public:
    // Correct values for packet_size_factor: 1 (10Hz refresh rate), 2 (20Hz), 3 (30Hz)
    ebu_r128(int sample_rate, const std::vector<Speaker>& channels, int packet_size_factor = 1)
        : m_sample_rate(sample_rate), m_running(true), m_need_reset(false),
          m_packet_size(sample_rate / 10 / packet_size_factor)
    {
        KFR_LOGIC_CHECK(!channels.empty(), "channels must not be empty");
        KFR_LOGIC_CHECK(sample_rate > 0, "sample_rate must be greater than 0");
        KFR_LOGIC_CHECK(packet_size_factor >= 1 && packet_size_factor <= 6,
                        "packet_size_factor must be in range [1..6]");
        for (Speaker sp : channels)
        {
            m_channels.emplace_back(sample_rate, sp, packet_size_factor, T(1));
        }
    }

    int sample_rate() const { return m_sample_rate; }

    size_t packet_size() const { return m_packet_size; }

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

    const ebu_channel<T>& operator[](size_t index) const { return m_channels[index]; }
    size_t count() const { return m_channels.size(); }

    void process_packet(const std::initializer_list<univector_dyn<T>>& source)
    {
        process_packet<tag_dynamic_vector>(source);
    }
    void process_packet(const std::initializer_list<univector_ref<T>>& source)
    {
        process_packet<tag_array_ref>(source);
    }

    template <univector_tag Tag>
    void process_packet(const std::vector<univector<T, Tag>>& source)
    {
        T momentary = 0;
        T shortterm = 0;
        for (size_t ch = 0; ch < m_channels.size(); ch++)
        {
            TESTO_ASSERT(source[ch].size() == m_packet_size);
            ebu_channel<T>& chan = m_channels[ch];
            chan.process_packet(source[ch].data());
            if (m_running)
            {
                momentary += mean(m_channels[ch].m_momentary_sum_of_squares) / m_packet_size *
                             m_channels[ch].m_output_energy_gain;
                shortterm += mean(m_channels[ch].m_short_sum_of_squares) / m_packet_size *
                             m_channels[ch].m_output_energy_gain;
            }
        }

        if (m_need_reset)
        {
            m_need_reset = false;
            for (size_t ch = 0; ch < m_channels.size(); ch++)
            {
                m_channels[ch].reset();
            }
            m_integrated_buffer.reset();
            m_lra_buffer.reset();
        }
        if (m_running)
        {
            m_integrated_buffer.push(momentary);
            m_lra_buffer.push(shortterm);
        }
    }

    void start() { m_running = true; }
    void stop() { m_running = false; }
    void reset() { m_need_reset = true; }

private:
    int m_sample_rate;
    bool m_running;
    bool m_need_reset;
    size_t m_packet_size;
    std::vector<ebu_channel<T>> m_channels;
    integrated_vec<T> m_integrated_buffer;
    lra_vec<T> m_lra_buffer;
};

} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
