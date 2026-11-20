/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <algorithm>
#include <array>
#include <charconv>
#include <exception>
#include <filesystem>
#include <cstdio>
#include <string_view>
#include <utility>
#include <vector>
#include <kfr/audio.hpp>
#include <kfr/base.hpp>
#include <kfr/meta/string.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp/units.hpp>
#include <kfr/dsp/window.hpp>

namespace
{
constexpr std::array windows{
    std::pair{ std::string_view("rectangular"), kfr::window_type::rectangular },
    std::pair{ std::string_view("hann"), kfr::window_type::hann },
    std::pair{ std::string_view("hamming"), kfr::window_type::hamming },
    std::pair{ std::string_view("blackman"), kfr::window_type::blackman },
    std::pair{ std::string_view("blackman_harris"), kfr::window_type::blackman_harris },
    std::pair{ std::string_view("flattop"), kfr::window_type::flattop },
};

bool parse_size(std::string_view text, std::size_t& value)
{
    const auto end          = text.data() + text.size();
    const auto [ptr, error] = std::from_chars(text.data(), end, value);
    return error == std::errc{} && ptr == end;
}

using pixel = std::array<unsigned char, 3>;
static_assert(sizeof(pixel) == 3);

pixel color(double magnitude)
{
    constexpr double floor_db = -120.0;
    const double db           = magnitude > 0.0 ? kfr::amp_to_dB(magnitude) : floor_db;
    const double level        = 3.0 * kfr::clamp((db - floor_db) / -floor_db, 0.0, 1.0);
    const auto component      = [](double x)
    { return static_cast<unsigned char>(kfr::round(255.0 * kfr::clamp(x, 0.0, 1.0))); };
    // TGA stores blue, green, then red. Each successive segment adds one channel.
    return { component(level - 2.0), component(level - 1.0), component(level) };
}

bool write_tga(const std::filesystem::path& path, std::size_t width, std::size_t height,
               const std::vector<pixel>& image)
{
    std::array<unsigned char, 18> header{};
    header[2]         = 2; // Uncompressed true color.
    header[12]        = static_cast<unsigned char>(width & 0xff);
    header[13]        = static_cast<unsigned char>(width >> 8);
    header[14]        = static_cast<unsigned char>(height & 0xff);
    header[15]        = static_cast<unsigned char>(height >> 8);
    header[16]        = 24;
    header[17]        = 0x20; // Top-left origin.
    const auto opened = kfr::fopen_path(kfr::file_path(path.native()), kfr::open_file_mode::write_new);
    if (!opened)
        return false;
    auto* file         = *opened;
    const bool written = std::fwrite(header.data(), 1, header.size(), file) == header.size() &&
                         std::fwrite(image.data(), sizeof(pixel), image.size(), file) == image.size();
    return std::fclose(file) == 0 && written;
}

int spectrogram(const char* audio_path, std::size_t width, std::size_t height, std::size_t fft_size,
                kfr::window_type window_type)
{
    const std::filesystem::path input_path(audio_path);
    auto output_path = input_path;
    output_path.replace_extension(".tga");
    if (input_path == output_path ||
        (std::filesystem::exists(input_path) && std::filesystem::exists(output_path) &&
         std::filesystem::equivalent(input_path, output_path)))
    {
        kfr::error("Output path would overwrite the input file\n");
        return 1;
    }

    kfr::audiofile_format format{};
    const auto audio = kfr::decode_audio_file(audio_path, &format);
    if (!audio)
    {
        kfr::errorln("Could not decode input: ", kfr::to_string(audio.error()));
        return 1;
    }
    const auto channels = audio->channel_count();
    const auto frames   = audio->size;
    if (frames == 0 || channels == 0)
    {
        kfr::error("Input has no audio samples\n");
        return 1;
    }
    const auto samples = audio->interleaved();

    const double parameter = window_type == kfr::window_type::hamming ? 0.54 : 0.16;
    const kfr::univector<double> window =
        kfr::window<double>(fft_size, window_type, parameter, kfr::window_symmetry::periodic);
    const double window_sum = kfr::sum(window);
    kfr::dft_plan_real<double> plan(fft_size, kfr::dft_pack_format::CCs);
    kfr::univector<kfr::u8> scratch(plan.temp_size);
    kfr::univector<kfr::complex<double>> spectrum(plan.complex_size());
    kfr::univector<double> frame(fft_size);
    kfr::univector<double> magnitudes(spectrum.size());
    std::vector<pixel> image;
    if (height > image.max_size() / width)
    {
        kfr::error("Image is too large\n");
        return 1;
    }
    image.resize(width * height);

    for (std::size_t x = 0; x < width; ++x)
    {
        // Center one window on each time column and zero-pad beyond the recording.
        const auto center       = static_cast<std::size_t>(((x + 0.5) / width) * frames);
        const auto half         = fft_size / 2;
        const auto first_sample = center >= half ? center - half : 0;
        const auto first_slot   = center >= half ? 0 : half - center;
        const auto count        = kfr::min(fft_size - first_slot, frames - first_sample);
        magnitudes              = kfr::scalar(0);

        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            frame = kfr::scalar(0);
            for (std::size_t i = 0; i < count; ++i)
            {
                const double sample = samples[(first_sample + i) * channels + channel];
                if ((kfr::abs(sample) < kfr::c_infinity<double>))
                    frame[first_slot + i] = sample * window[first_slot + i];
            }
            plan.execute(spectrum, frame, scratch);
            for (std::size_t bin = 0; bin < spectrum.size(); ++bin)
            {
                // A full-scale bin-centered sine reads 0 dBFS. DC and Nyquist have no mirror bin.
                const bool endpoint    = bin == 0 || (fft_size % 2 == 0 && bin == half);
                const double magnitude = kfr::cabs(spectrum[bin]) * (endpoint ? 1.0 : 2.0) / window_sum;
                // Use the strongest channel per bin to avoid phase cancellation between channels.
                magnitudes[bin] = kfr::max(magnitudes[bin], magnitude);
            }
        }

        for (std::size_t y = 0; y < height; ++y)
        {
            // Linear bands from DC to Nyquist. Keep the strongest bin in each band when shrinking.
            const auto low  = static_cast<std::size_t>(kfr::ceil((0.5 * fft_size * y) / height));
            const auto high = y + 1 == height
                                  ? magnitudes.size()
                                  : static_cast<std::size_t>(kfr::ceil((0.5 * fft_size * (y + 1)) / height));
            const auto nearest = static_cast<std::size_t>(kfr::round((0.5 * fft_size * (y + 0.5)) / height));
            const double magnitude = low < high ? kfr::maxof(magnitudes.slice(low, high - low))
                                                : magnitudes[kfr::min(nearest, magnitudes.size() - 1)];
            image[(height - 1 - y) * width + x] = color(magnitude);
        }
    }

    if (!write_tga(output_path, width, height, image))
    {
        kfr::errorln("Could not write ", output_path.string());
        return 1;
    }
    kfr::print("Wrote ", width, "x", height, " spectrogram to ", output_path.string(), " (0 to ",
               format.sample_rate / 2.0, " Hz)\n");
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    if (argc == 5)
    {
        const std::string_view dimensions(argv[2]);
        const auto separator = dimensions.find_first_of("xX");
        const auto window    = std::find_if(windows.begin(), windows.end(),
                                            [&](const auto& entry) { return entry.first == argv[4]; });
        std::size_t width, height, fft_size;
        if (separator != std::string_view::npos && parse_size(dimensions.substr(0, separator), width) &&
            parse_size(dimensions.substr(separator + 1), height) && width > 0 && width <= 65535 &&
            height > 0 && height <= 65535 && parse_size(argv[3], fft_size) && fft_size >= 4 &&
            fft_size <= 1048576 && window != windows.end())
        {
            try
            {
                return spectrogram(argv[1], width, height, fft_size, window->second);
            }
            catch (const std::exception& error)
            {
                kfr::errorln("Spectrogram failed: ", error.what());
                return 1;
            }
        }
    }
    kfr::error("Usage: spectrogram AudioFile WidthxHeight FftSize WindowFunction\n",
               "Width and height: 1..65535; FFT size: 4..1048576.\nWindow functions:");
    for (const auto& entry : windows)
        kfr::error(" ", entry.first);
    kfr::errorln();
    return 1;
}
