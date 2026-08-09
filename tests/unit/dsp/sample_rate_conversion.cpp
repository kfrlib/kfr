/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/base/math_expressions.hpp>
#include <kfr/dsp/oscillators.hpp>
#include <kfr/dsp/sample_rate_conversion.hpp>
#include <kfr/dsp/special.hpp>
#include <kfr/math/sin_cos.hpp>

#include <cmath>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
namespace
{
constexpr size_t test_duration_tenths                = 1;
constexpr fbase rms_tolerance                        = 0.001f;
constexpr fbase max_tolerance                        = 0.005f;
constexpr fbase chunked_chirp_passband_rms_tolerance = choose_const<fbase>(0.002f, 0.001);
constexpr fbase chunked_chirp_passband_max_tolerance = choose_const<fbase>(0.02f, 0.005);
constexpr fbase chunked_chirp_stopband_rms_tolerance = choose_const<fbase>(0.01f, 0.0001);
constexpr fbase chunked_chirp_stopband_max_tolerance = choose_const<fbase>(0.06f, 0.0005);

univector<fbase> sine(int frequency, int sample_rate, size_t size, double delay = 0)
{
    return truncate(sin(c_pi<fbase> * phasor<fbase>(frequency, sample_rate,
                                                    -delay * (static_cast<double>(frequency) / sample_rate))),
                    size);
}

void check_sine_error(const univector<fbase>& output, const univector<fbase>& reference, size_t warmup)
{
    const auto error = slice(output - reference, warmup);
    CHECK(rms(error) < rms_tolerance);
    CHECK(absmaxof(error) < max_tolerance);
}

void check_resampling(int input_rate, int output_rate, int frequency)
{
    const size_t input_size      = input_rate / test_duration_tenths;
    const size_t output_size     = output_rate / test_duration_tenths;
    const univector<fbase> input = sine(frequency, input_rate, input_size);

    auto reference_converter =
        sample_rate_converter<fbase>(resample_quality::normal, output_rate, input_rate);
    const double delay               = reference_converter.get_fractional_delay();
    const size_t warmup              = static_cast<size_t>(ceil(delay * 2));
    const univector<fbase> reference = sine(frequency, output_rate, output_size, delay);

    CAPTURE(input_rate, output_rate, frequency, delay);

    SECTION("one-shot")
    {
        auto converter = sample_rate_converter<fbase>(resample_quality::normal, output_rate, input_rate);
        univector<fbase> output(output_size);

        CHECK(converter.process(output, input) == input_size);
        check_sine_error(output, reference, warmup);
    }

    SECTION("pull one output sample at a time")
    {
        auto converter = sample_rate_converter<fbase>(resample_quality::normal, output_rate, input_rate);
        univector<fbase> output(output_size);
        size_t input_position = 0;

        for (size_t output_position = 0; output_position < output.size(); ++output_position)
        {
            const size_t requested_input = static_cast<size_t>(converter.input_size_for_output(1));
            univector<fbase> output_sample(1);

            REQUIRE(input_position + requested_input <= input.size());
            CHECK(converter.process(output_sample, input.slice(input_position, requested_input)) ==
                  requested_input);
            output[output_position] = output_sample[0];
            input_position += requested_input;
        }

        CHECK(input_position == input.size());
        check_sine_error(output, reference, warmup);
    }

    SECTION("push one input sample at a time")
    {
        auto converter = sample_rate_converter<fbase>(resample_quality::normal, output_rate, input_rate);
        univector<fbase> output(output_size);
        size_t input_position  = 0;
        size_t output_position = 0;
        size_t pending_input   = 0;

        while (input_position + pending_input < input.size())
        {
            ++pending_input;
            const size_t requested_output =
                static_cast<size_t>(converter.output_size_for_input(pending_input));

            if (requested_output == 0)
                continue;

            univector<fbase> output_chunk(requested_output);
            REQUIRE(output_position + requested_output <= output.size());
            CHECK(converter.process(output_chunk, input.slice(input_position, pending_input)) ==
                  pending_input);
            for (size_t index = 0; index < requested_output; ++index)
                output[output_position + index] = output_chunk[index];

            input_position += pending_input;
            output_position += requested_output;
            pending_input = 0;
        }

        CHECK(pending_input == 0);
        CHECK(input_position == input.size());
        CHECK(output_position == output.size());
        check_sine_error(output, reference, warmup);
    }
}

void check_chirp_resampling(int input_rate, int output_rate)
{
    const size_t input_size      = input_rate / test_duration_tenths;
    const size_t output_size     = output_rate / test_duration_tenths;
    const univector<fbase> input = render(swept<fbase>(1, input_size));

    auto converter = sample_rate_converter<fbase>(resample_quality::normal, output_rate, input_rate);
    univector<fbase> output(output_size);

    CHECK(converter.process(output, input) == input_size);

    const size_t warmup = static_cast<size_t>(ceil(converter.get_fractional_delay() * 2));
    const auto signal   = slice(output, warmup);
    CAPTURE(input_rate, output_rate, converter.get_fractional_delay());

    // A swept sine visits the entire retained band and must retain substantial energy.
    CHECK(rms(signal) > 0.1f);
    CHECK(absmaxof(signal) > 0.5f);
}

univector<fbase> chirp_reference(size_t output_size, size_t input_size, int input_rate, int output_rate,
                                 double delay)
{
    univector<fbase> result(output_size);
    const double phase_scale = constants<fbase>::pi * 0.25 / cub(static_cast<double>(input_size));

    for (size_t index = 0; index < result.size(); ++index)
    {
        const double input_position =
            (static_cast<double>(index) - delay) * static_cast<double>(input_rate) / output_rate;
        result[index] =
            0.5 * std::sin(phase_scale * input_position * input_position * input_position * input_position);
    }
    return result;
}

void check_chunked_chirp_resampling()
{
    constexpr int input_rate           = 96000;
    constexpr int output_rate          = 44100;
    constexpr size_t duration_seconds  = 8;
    constexpr size_t output_chunk_size = 16384;
    constexpr fbase passband_end       = 18000;
    constexpr fbase stopband_start     = 26000;

    const size_t input_size      = input_rate * duration_seconds;
    const univector<fbase> input = render(swept<fbase>(0.5, input_size));
    auto converter = sample_rate_converter<fbase>(resample_quality::high, output_rate, input_rate);

    const size_t output_delay  = converter.get_delay();
    const size_t skipped_input = converter.input_size_for_output(output_delay);
    const size_t output_size =
        static_cast<size_t>(converter.output_size_for_input(input_size)) - output_delay;
    univector<fbase> output(output_size);

    REQUIRE(skipped_input <= input.size());
    CHECK(converter.skip(output_delay, input.slice(0, skipped_input)) == skipped_input);

    size_t input_position  = skipped_input;
    size_t output_position = 0;
    while (output_position < output.size())
    {
        const size_t requested_output = std::min(output_chunk_size, output.size() - output_position);
        const size_t requested_input = static_cast<size_t>(converter.input_size_for_output(requested_output));
        univector<fbase> output_chunk(requested_output);

        REQUIRE(input_position + requested_input <= input.size());
        CHECK(converter.process(output_chunk, input.slice(input_position, requested_input)) ==
              requested_input);
        for (size_t index = 0; index < output_chunk.size(); ++index)
            output[output_position + index] = output_chunk[index];

        input_position += requested_input;
        output_position += requested_output;
    }

    CHECK(input_position == input.size());
    const univector<fbase> reference = chirp_reference(output_size, input_size, input_rate, output_rate,
                                                       converter.get_fractional_delay() - output_delay);

    const size_t passband_size =
        static_cast<size_t>(output_size * std::cbrt(static_cast<double>(passband_end) / (input_rate * 0.5)));
    const size_t stopband_start_index = static_cast<size_t>(
        output_size * std::cbrt(static_cast<double>(stopband_start) / (input_rate * 0.5)));
    const auto passband = output.slice(0, passband_size) - reference.slice(0, passband_size);
    const auto stopband = output.slice(stopband_start_index);

    CAPTURE(output_delay, converter.get_fractional_delay(), passband_size, stopband_start_index);
    CHECK(rms(passband) < chunked_chirp_passband_rms_tolerance);
    CHECK(absmaxof(passband) < chunked_chirp_passband_max_tolerance);
    CHECK(rms(stopband) < chunked_chirp_stopband_rms_tolerance);
    CHECK(absmaxof(stopband) < chunked_chirp_stopband_max_tolerance);
}
} // namespace

TEST_CASE("sample rate conversion")
{
    check_resampling(44100, 48000, 100);
    check_resampling(44100, 48000, 1000);
    check_resampling(48000, 44100, 100);
    check_resampling(48000, 44100, 8000);
    check_resampling(48000, 32000, 1000);
    check_resampling(32000, 48000, 6000);
}

TEST_CASE("sample rate conversion logarithmic sweep")
{
    check_chirp_resampling(44100, 48000);
    check_chirp_resampling(48000, 44100);
    check_chirp_resampling(48000, 32000);
}

TEST_CASE("sample rate conversion chunked chirp") { check_chunked_chirp_resampling(); }

} // namespace KFR_ARCH_NAME
} // namespace kfr
