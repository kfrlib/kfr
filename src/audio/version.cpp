#include <kfr/cident.h>
#if !defined KFR_SKIP_IF_NON_X86 || defined(KFR_ARCH_X86)

#include <kfr/kfr.h>

namespace kfr
{
const char* library_version_audio() { return KFR_VERSION_FULL; }
const char* library_version_codecs()
{
    return "wav,mp3,w64,aiff,caf,rf64,bw64,raw" // Always enabled codecs
    // Comma-separate list of enabled codecs
#if defined(KFR_AUDIO_FLAC) && KFR_AUDIO_FLAC
           ",flac"
#endif
#if defined(KFR_AUDIO_ALAC) && KFR_AUDIO_ALAC
           ",alac"
#endif
        ;
}
} // namespace kfr

#endif
