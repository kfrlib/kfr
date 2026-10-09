#include <kfr/all.hpp>

using namespace kfr;

int main()
{
    println(library_version());
    println("DFT: ", library_version_dft());
    println("DSP: ", library_version_dsp());
    println("IO: ", library_version_io());
    println("Audio: ", library_version_audio());
    println("Codecs: ", library_version_codecs());
    return 0;
}
