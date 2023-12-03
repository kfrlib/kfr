/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/cometa.hpp>
#include <kfr/cometa/ctti.hpp>
#include <kfr/testo/testo.hpp>

namespace kfr
{

TEST(ctti)
{
    CHECK(cometa::type_name<float>() == std::string("float"));
    CHECK(cometa::type_name<int>() == std::string("int"));
}
} // namespace kfr
