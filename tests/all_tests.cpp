/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>
#include <kfr/test/test.hpp>
#include <kfr/version.hpp>

using namespace kfr;

int main(int argc, char* argv[])
{
    println(library_version(), " running on ", cpu_runtime());

    int result = Catch::Session().run(argc, argv);

    return result;
}
