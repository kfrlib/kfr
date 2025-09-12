/** @addtogroup audio
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
#ifdef _WIN32

// This file implements an audio decoder using Windows Media Foundation API

#include <kfr/audio/decoder.hpp>

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <stdio.h>
#include <mferror.h>
#include <Windows.Foundation.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <propvarutil.h>

namespace kfr
{

using Microsoft::WRL::ComPtr;

struct MFDecoder : public audio_decoder
{
public:
    MFDecoder(mediafoundation_decoding_options options) : options(std::move(options)) {}
    [[nodiscard]] expected<audiofile_metadata, audiofile_error> open(const file_path& path) override;
    [[nodiscard]] expected<audio_data, audiofile_error> read() override;
    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override;
    void close() override;

protected:
    ComPtr<IMFSourceReader> pReader;
    ComPtr<IMFMediaType> ppPCMAudio;
    mediafoundation_decoding_options options;
};

std::unique_ptr<audio_decoder> create_mediafoundation_decoder(const mediafoundation_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new MFDecoder(options));
}

static audiofile_error map_hresult_to_error(HRESULT hr)
{
    switch (hr)
    {
    case MF_E_END_OF_STREAM:
        return audiofile_error::end_of_file;
    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
    case HRESULT_FROM_WIN32(ERROR_HANDLE_DISK_FULL):
    case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
        return audiofile_error::io_error;
    case MF_E_UNSUPPORTED_BYTESTREAM_TYPE:
    case MF_E_INVALID_STREAM_DATA:
    case MF_E_UNSUPPORTED_RATE:
        return audiofile_error::format_error;
    default:
        return audiofile_error::internal_error;
    }
}

#define HANDLE_ERROR(...)                                                                                    \
    do                                                                                                       \
    {                                                                                                        \
        HRESULT hr = __VA_ARGS__;                                                                            \
        if (FAILED(hr))                                                                                      \
        {                                                                                                    \
            return unexpected(map_hresult_to_error(hr));                                                     \
        }                                                                                                    \
    } while (0)

using MFSample = float;

std::string wstring_to_utf8(std::wstring_view wstr)
{
    if (wstr.empty())
        return {};

    int needed = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.data(),
                                       static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);

    if (needed == 0)
    {
        return {};
    }

    std::string result;
    result.resize(static_cast<size_t>(needed));

    int converted =
        ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.data(), static_cast<int>(wstr.size()),
                              result.data(), needed, nullptr, nullptr);

    if (converted == 0)
    {
        return {};
    }
    return result;
}

static std::map<std::string, std::string> readMeta(ComPtr<IMFSourceReader> pReader)
{
    std::map<std::string, std::string> metadata;

    if (!pReader)
    {
        return metadata;
    }

    ComPtr<IMFMediaSource> spSource;
    ComPtr<IMFPresentationDescriptor> spPD;
    ComPtr<IMFMetadataProvider> spMetadataProvider;
    ComPtr<IMFMetadata> spMetadata;

    // A better way: get service from the media source
    HRESULT hr =
        pReader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, GUID_NULL, IID_PPV_ARGS(&spSource));

    if (FAILED(hr) || !spSource)
    {
        return metadata;
    }

    // Create Presentation Descriptor
    hr = spSource->CreatePresentationDescriptor(&spPD);
    if (FAILED(hr))
    {
        return metadata;
    }

    // Get MetadataProvider
    hr = MFGetService(spSource.Get(), MF_METADATA_PROVIDER_SERVICE, IID_PPV_ARGS(&spMetadataProvider));
    if (FAILED(hr))
    {
        return metadata;
    }

    // Get Metadata
    hr = spMetadataProvider->GetMFMetadata(spPD.Get(), 0, 0, &spMetadata);
    if (FAILED(hr))
    {
        return metadata;
    }

    // Enumerate metadata items
    PROPVARIANT varNames;
    PropVariantInit(&varNames);

    hr = spMetadata->GetAllPropertyNames(&varNames);
    if (SUCCEEDED(hr) && varNames.vt == (VT_VECTOR | VT_LPWSTR))
    {
        for (ULONG i = 0; i < varNames.calpwstr.cElems; i++)
        {
            LPCWSTR key = varNames.calpwstr.pElems[i];
            PROPVARIANT varVal;
            PropVariantInit(&varVal);

            if (SUCCEEDED(spMetadata->GetProperty(key, &varVal)))
            {
                std::string sKey = wstring_to_utf8(key);

                std::string sVal;
                if (varVal.vt == VT_LPWSTR && varVal.pwszVal)
                {
                    sVal = wstring_to_utf8(varVal.pwszVal);
                }
                else if (varVal.vt == VT_BSTR && varVal.bstrVal)
                {
                    sVal = wstring_to_utf8(varVal.bstrVal);
                }
                else if (varVal.vt == VT_UI4)
                {
                    sVal = std::to_string(varVal.ulVal);
                }

                if (!sKey.empty() && !sVal.empty())
                {
                    metadata[sKey] = sVal;
                }
            }

            PropVariantClear(&varVal);
        }
    }

    PropVariantClear(&varNames);

    return metadata;
}

expected<audiofile_metadata, audiofile_error> MFDecoder::open(const file_path& path)
{
    ComPtr<IMFMediaType> pSourceAudioType;
    ComPtr<IMFMediaType> pUncompressedAudioType;
    ComPtr<IMFMediaType> pPartialType;

    static std::once_flag flag;

    HRESULT startup_hr = S_OK;
    std::call_once(flag, [&]() { startup_hr = MFStartup(MF_VERSION); });
    HANDLE_ERROR(startup_hr);

    HANDLE_ERROR(MFCreateSourceReaderFromURL(path.c_str(), NULL, pReader.ReleaseAndGetAddressOf()));
    HANDLE_ERROR(pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE));
    HANDLE_ERROR(pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE));

    HANDLE_ERROR(pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                              pSourceAudioType.ReleaseAndGetAddressOf()));

    audiofile_metadata info;
    info.container   = audiofile_container::unknown;
    info.codec       = audiofile_codec::unknown;
    info.endianness  = audiofile_endianness::little;
    info.bit_depth   = MFGetAttributeUINT32(pSourceAudioType.Get(), MF_MT_AUDIO_BITS_PER_SAMPLE, 0);
    info.channels    = MFGetAttributeUINT32(pSourceAudioType.Get(), MF_MT_AUDIO_NUM_CHANNELS, 0);
    info.sample_rate = MFGetAttributeUINT32(pSourceAudioType.Get(), MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
    if (info.channels == 0 || info.sample_rate == 0)
        return unexpected(audiofile_error::format_error);

    PROPVARIANT var;
    LONGLONG hnsDuration = 0;
    HANDLE_ERROR(pReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var));
    HANDLE_ERROR(PropVariantToInt64(var, &hnsDuration));
    PropVariantClear(&var);
    // info.totalSamples = muldiv(hnsDuration, info.sample_rate, 10'000'000);
    info.total_frames = 1.0 * hnsDuration * info.sample_rate / 10'000'000.0;

    HANDLE_ERROR(MFCreateMediaType(pPartialType.ReleaseAndGetAddressOf()));
    HANDLE_ERROR(pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
    HANDLE_ERROR(pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float));

    size_t blockAlign = sizeof(MFSample) * info.channels;
    HANDLE_ERROR(pPartialType->SetUINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, TRUE));
    HANDLE_ERROR(pPartialType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, info.channels));
    HANDLE_ERROR(pPartialType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, info.sample_rate));
    HANDLE_ERROR(pPartialType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, blockAlign));
    HANDLE_ERROR(pPartialType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, blockAlign * info.sample_rate));
    HANDLE_ERROR(pPartialType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, sizeof(MFSample) * 8));
    HANDLE_ERROR(pPartialType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));

    HANDLE_ERROR(
        pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pPartialType.Get()));
    HANDLE_ERROR(pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                              pUncompressedAudioType.ReleaseAndGetAddressOf()));
    HANDLE_ERROR(pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE));

    ppPCMAudio = pUncompressedAudioType;

    m_metadata = info;

    if (options.read_metadata)
    {
        // Read media metadata (author, title, album, etc) using MF
        m_metadata->metadata = readMeta(pReader);
    }
    return info;
}
expected<audio_data, audiofile_error> MFDecoder::read()
{
    ComPtr<IMFSample> pSample;
    ComPtr<IMFMediaBuffer> pBuffer;
    DWORD dwFlags    = 0;
    BYTE* pAudioData = NULL;
    DWORD cbBuffer   = 0;

    KFR_ASSERT(m_metadata.has_value());

    audio_data data{ &*m_metadata };

    for (;;)
    {
        HANDLE_ERROR(pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL,
                                         pSample.ReleaseAndGetAddressOf()));
        if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            return unexpected(audiofile_error::end_of_file);
        }
        if (!pSample)
            continue;

        HANDLE_ERROR(pSample->ConvertToContiguousBuffer(pBuffer.ReleaseAndGetAddressOf()));
        HANDLE_ERROR(pBuffer->Lock(&pAudioData, NULL, &cbBuffer));
        size_t numSamples = cbBuffer / sizeof(MFSample) / m_metadata->channels;
        if (numSamples == 0)
            continue;
        data.resize(numSamples);
        deinterleave_samples(data.pointers(), reinterpret_cast<const MFSample*>(pAudioData),
                             m_metadata->channels, numSamples);

        HANDLE_ERROR(pBuffer->Unlock());
        pAudioData = nullptr;

        pBuffer = nullptr; // release

        break;
    }

    return data;
}
expected<void, audiofile_error> MFDecoder::seek(uint64_t position)
{
    uint64_t hnsPosition = position * 10'000'000.0 / m_metadata->sample_rate;

    PROPVARIANT var;
    HANDLE_ERROR(InitPropVariantFromInt64(hnsPosition, &var));
    HANDLE_ERROR(pReader->SetCurrentPosition(GUID_NULL, var));
    PropVariantClear(&var);

    return {};
}

void MFDecoder::close()
{
    ppPCMAudio.Reset();
    pReader.Reset();
    m_metadata.reset();
}
} // namespace kfr

#endif
