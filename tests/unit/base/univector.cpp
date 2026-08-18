/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/io/tostring.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

TEST_CASE("univector_assignment")
{
    univector<int> x = truncate(counter(), 10);
    CHECK(x.size() == 10u);

    univector<int> y;
    y = truncate(counter(), 10);
    CHECK(y.size() == 10u);
}

TEST_CASE("univector_ringbuf_oversized_transfers")
{
    univector<int, 4> buffer{ 0, 0, 0, 0 };
    const int source[] = { 1, 2, 3, 4, 5, 6 };
    size_t cursor      = 2;

    buffer.ringbuf_write(cursor, source, std::size(source));
    CHECK(cursor == 2u);
    CHECK(buffer[0] == 5);
    CHECK(buffer[1] == 6);
    CHECK(buffer[2] == 3);
    CHECK(buffer[3] == 4);

    int destination[] = { -1, -1, -1, -1, -1, -1 };
    buffer.ringbuf_read(cursor, destination, std::size(destination));
    CHECK(cursor == 2u);
    CHECK(destination[0] == -1);
    CHECK(destination[1] == -1);
    CHECK(destination[2] == 3);
    CHECK(destination[3] == 4);
    CHECK(destination[4] == 5);
    CHECK(destination[5] == 6);
}

TEST_CASE("univector_ringbuf_empty_buffer")
{
    univector<int> buffer;
    size_t cursor = 0;
    int value     = 42;

    buffer.ringbuf_write(cursor, value);
    buffer.ringbuf_read(cursor, value);
    CHECK(cursor == 0u);
    CHECK(value == 42);
}

TEST_CASE("spsc_ring_buffer_transfers")
{
    static_assert(std::is_same_v<lockfree_ring_buffer<int>, spsc_ring_buffer<int>>);

    univector<int, 4> storage{ 0, 0, 0, 0 };
    spsc_ring_buffer<int> queue;
    const int input[] = { 1, 2, 3, 4, 5 };
    int output[3]     = {};

    CHECK(queue.try_enqueue(input, 3, storage) == 3u);
    CHECK(queue.size() == 3u);
    CHECK(queue.try_enqueue(input + 3, 2, storage) == 0u);
    CHECK(queue.try_dequeue(output, 2, storage) == 2u);
    CHECK(output[0] == 1);
    CHECK(output[1] == 2);
    CHECK(queue.try_enqueue(input + 3, 2, storage) == 2u);
    CHECK(queue.try_dequeue(output, 3, storage) == 3u);
    CHECK(output[0] == 3);
    CHECK(output[1] == 4);
    CHECK(output[2] == 5);
    CHECK(queue.size() == 0u);
}

TEST_CASE("spsc_ring_buffer_empty_transfers")
{
    univector<int> storage;
    spsc_ring_buffer<int> queue;

    CHECK(queue.try_enqueue(nullptr, 0, storage) == 0u);
    CHECK(queue.try_dequeue(nullptr, 0, storage) == 0u);
}

#ifdef KFR_USE_STD_ALLOCATION
TEST_CASE("std_allocation")
{
    univector<float> u;
    std::vector<float>& v = u;

    std::vector<float> v2{ 1, 2, 3, 4 };

    // Technically an UB but ok with all sane compilers
    reinterpret_cast<univector<float>&>(v2) += 100.f;
    CHECK(v2[0] == 101);
    CHECK(v2[1] == 102);
    CHECK(v2[2] == 103);
    CHECK(v2[3] == 104);
}
#endif

} // namespace KFR_ARCH_NAME
} // namespace kfr
