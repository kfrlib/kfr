/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/graphics/geometry.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(point)
{
    testo::epsilon_scope<void> e(100);

    f32point p{ 0.f, 0.5f };
    CHECK(p.distance(i32point{ 1, 2 }) == 1.80277563773f);
    CHECK(i32point{ 1, 2 } + i32point{ 4, -4 } == i32point{ 5, -2 });
    CHECK(i32point{ 1, 2 } * 4 == i32point{ 4, 8 });

    CHECK(i32point(f32point{ 0.25f, 0.75f }) == i32point{ 0, 1 });

    CHECK(f32point{ 0, 0 }.aligned_rect({ 10, 10 }, { 0.5f, 0.5f }) == f32rectangle{ -5, -5, 5, 5 });
    CHECK(f32point{ 0, 0 }.aligned_rect({ 10, 10 }, { 0.f, 0.f }) == f32rectangle{ 0, 0, 10, 10 });
    CHECK(f32point{ 0, 0 }.aligned_rect({ 9, 9 }, { 0.5f, 0.5f }) ==
          f32rectangle{ -4.5f, -4.5f, 4.5f, 4.5f });
    CHECK(f32point{ 5, 2 }.aligned_rect({ 10, 10 }, { 0.5f, 0.5f }) == f32rectangle{ 0, -3, 10, 7 });
}

TEST(size)
{
    CHECK(i32size(f32size{ 0.25f, 0.75f }) == i32size{ 0, 1 });
    CHECK(i32size{ 1, 2 } + i32size{ 4, -4 } == i32size{ 5, -2 });
    CHECK(i32size{ 1, 2 } * 4 == i32size{ 4, 8 });
    CHECK(i32size{ 1, 2 }.area() = 2);
}

TEST(border)
{
    CHECK(i32border{ 1, 2, 3, 4 }.size() == i32size{ 4, 6 });
    CHECK(i32border{ 1, 2, 3, 4 }.leading() == i32size{ 1, 2 });
    CHECK(i32border{ 1, 2, 3, 4 }.trailing() == i32size{ 3, 4 });
    CHECK(i32border{ 1, 2, 3, 4 }.horizontal() == 4);
    CHECK(i32border{ 1, 2, 3, 4 }.vertical() == 6);
}

TEST(rectangle)
{
    testo::epsilon_scope<void> e(100);
    CHECK(f32rectangle{ f32point{ 1, 2 }, f32size{ 2, 2 } } == f32rectangle{ 1, 2, 3, 4 });
    CHECK(f32rectangle{ f32point{ 1, 2 }, f32point{ 3, 4 } } == f32rectangle{ 1, 2, 3, 4 });
    CHECK(f32rectangle{ f32point{ 1, 2 }, f32size{ 3, 4 }, f32point{ 0.5f, 0.5f } } ==
          f32rectangle{ -0.5f, 0, 2.5f, 4 });

    CHECK(!i32rectangle{ i32point{ 0, 0 }, i32size{ 3, 1 } }.empty());
    CHECK(i32rectangle{ i32point{ 0, 0 }, i32size{ 3, 0 } }.empty());
    CHECK(i32rectangle{ i32point{ 0, 0 }, i32size{ 0, 3 } }.empty());
    CHECK(i32rectangle{ i32point{ 4, 0 }, i32point{ 3, 101 } }.empty());
    CHECK(!i32rectangle{ i32point{ 3, 0 }, i32point{ 4, 101 } }.empty());
    CHECK(f32rectangle{ 1, 2, 7, 6 }.size() == f32size{ 6, 4 });
    CHECK(f32rectangle{ 1, 2, 7, 6 }.area() == 24);

    CHECK(f32rectangle{ 1, 2, 7, 6 }.min_side() == 4);
    CHECK(f32rectangle{ 1, 2, 7, 6 }.max_side() == 6);
    CHECK(f32rectangle{ 1, 2, 8, 6 }.center() == f32point{ 4.5f, 4.f });
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
