/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/meta.hpp>
#include <kfr/meta/ctti.hpp>
#include <kfr/testo/testo.hpp>

namespace kfr
{

TEST(ctti)
{
    CHECK(kfr::type_name<float>() == std::string("float"));
    CHECK(kfr::type_name<int>() == std::string("int"));
}
} // namespace kfr
