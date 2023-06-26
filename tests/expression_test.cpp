/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/cometa/function.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

TEST(univector_assignment)
{
    univector<int> x = truncate(counter(), 10);
    CHECK(x.size() == 10u);

    univector<int> y;
    y = truncate(counter(), 10);
    CHECK(y.size() == 10u);
}

TEST(mix)
{
    CHECK_EXPRESSION(mix(sequence(0, 0.5f, 1, 0.5f), counter(), counter() * 10), infinite_size,
                     [](size_t i) {
                         return mix(std::array<float, 4>{ 0, 0.5f, 1, 0.5f }[i % 4], i, i * 10);
                     });
}

TEST(expression_mask)
{
    univector<float> x(100);
    univector<float> y(100);
    x = select(x > y, 0.5f, 0.1f) * (y - x) + x;
}

} // namespace CMT_ARCH_NAME

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version());

    return testo::run_all("", true);
}
#endif
