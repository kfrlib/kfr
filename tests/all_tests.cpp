/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <cstdio>

#include <kfr/io/tostring.hpp>
#include <kfr/test/test.hpp>
#include <kfr/version.hpp>

using namespace kfr;

int main(int argc, char* argv[])
{
    // Unbuffered so output is visible immediately, even if a test crashes or hangs.
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    println(library_version(), " running on ", cpu_runtime());

    int result = Catch::Session().run(argc, argv);

    return result;
}
