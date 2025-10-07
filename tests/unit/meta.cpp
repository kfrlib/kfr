/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/meta.hpp>
#include <kfr/meta/ctti.hpp>
#include <kfr/test/test.hpp>

namespace kfr
{

TEST_CASE("ctti")
{
    CHECK(kfr::type_name<float>() == std::string("float"));
    CHECK(kfr::type_name<int>() == std::string("int"));
}
} // namespace kfr
