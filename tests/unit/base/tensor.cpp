/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/math_expressions.hpp>
#include <kfr/base/reduce.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/tensor.hpp>
#include <kfr/io/tostring.hpp>
#include <kfr/simd.hpp>

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 5051))

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

TEST(tensor_base)
{
    tensor<float, 2> t{ shape{ 20, 40 } };
    CHECK(t.shape() == shape{ 20, 40 });
    CHECK(t.strides() == shape{ 40, 1 });
    CHECK(t.size() == 800);
    CHECK(t.is_contiguous());

    t(0, 0) = 123;
    t(1, 1) = 456;

    tensor<float, 2> t2 = t(tstop(10), tstop(10));
    CHECK(t2.shape() == shape{ 10, 10 });
    CHECK(t2.strides() == shape{ 40, 1 });
    CHECK(t2.size() == 100);
    CHECK(!t2.is_contiguous());

    CHECK(t2(0, 0) == 123);
    CHECK(t2.data() == t.data());
    CHECK(t2.finalizer() == t.finalizer());

    tensor<float, 2> t3 = t(tstart(1), tstart(1));
    CHECK(t3.shape() == shape{ 19, 39 });
    CHECK(t3.strides() == shape{ 40, 1 });
    CHECK(t3.size() == 741);
    CHECK(!t3.is_contiguous());

    CHECK(t3(0, 0) == 456);
    CHECK(t3.data() == t.data() + 40 + 1);
    CHECK(t3.finalizer() == t.finalizer());
}

TEST(tensor_memory)
{
    // reference
    std::vector<float> vector{ 1.23f };
    tensor<float, 1> t{ vector.data(), shape{ 1 }, nullptr };
    CHECK(t.shape() == shape{ 1 });
    CHECK(t(0) == 1.23f);

    // adapt
    std::vector<float> vector2{ 2.34f };
    tensor<float, 1> t2 = tensor_from_container(std::move(vector2));
    CHECK(t2.shape() == shape{ 1 });
    CHECK(t2(0) == 2.34f);

    struct Container
    {
        std::array<double, 1> arr;
        int* refs;
        double* data() { return arr.data(); }
        size_t size() const { return arr.size(); }
        using value_type = double;
        Container(std::array<double, 1> arr, int* refs) : arr(arr), refs(refs) {}
        Container() { ++*refs; }
        Container(Container&& p) : arr(p.arr), refs(p.refs) { ++*refs; }
        Container(const Container&) = delete;
        ~Container() { --*refs; }
    };

    int refs = 0;
    Container cont{ { 3.45 }, &refs };
    {
        tensor<double, 1> t3 = tensor_from_container(std::move(cont));
        CHECK(t3.shape() == shape{ 1 });
        CHECK(t3(0) == 3.45);
        CHECK(refs == 1);
    }
    CHECK(refs == 0);
}

TEST(tensor_expression)
{
    tensor<float, 1> t1{ shape{ 32 }, 0.f };
    tensor<float, 1> t2{ shape{ 32 }, 100.f };
    tensor<float, 1> t3{ shape{ 32 }, 0.f };

    t1 = counter();

    CHECK(t1.size() == 32);
    CHECK(t1(0) == 0.f);
    CHECK(t1(1) == 1.f);
    CHECK(t1(31) == 31.f);

    t3 = t1 + t2;

    CHECK(t3.size() == 32);
    CHECK(t3(0) == 100.f);
    CHECK(t3(1) == 101.f);
    CHECK(t3(31) == 131.f);

    tensor<float, 2> t4{ shape{ 6, 6 }, 0.f };

    t4 = 1.f;
    CHECK(t4(0, 0) == 1.f);
    CHECK(t4(5, 5) == 1.f);
    CHECK(minof(t4) == 1);
    CHECK(maxof(t4) == 1);
    CHECK(sum(t4) == 36);

    t4(trange(2, 4), trange(2, 4)) = scalar(10);

    CHECK(t4(0, 0) == 1.f);
    CHECK(t4(1, 1) == 1.f);
    CHECK(t4(2, 2) == 10.f);
    CHECK(t4(3, 3) == 10.f);
    CHECK(t4(5, 5) == 1.f);
    CHECK(sum(t4) == 72);

    t4(trange(2, 4), trange(2, 4)) = 10 + counter(0, 2, 1);

    CHECK(t4(2, 2) == 10.f);
    CHECK(t4(2, 3) == 11.f);
    CHECK(t4(3, 2) == 12.f);
    CHECK(t4(3, 3) == 13.f);
    CHECK(sum(t4) == 78);
}

TEST(tensor_broadcast)
{
    tensor<float, 2> t1{ shape{ 1, 5 }, { 1.f, 2.f, 3.f, 4.f, 5.f } };
    tensor<float, 2> t2{ shape{ 5, 1 }, { 10.f, 20.f, 30.f, 40.f, 50.f } };
    tensor<float, 1> t4{ shape{ 5 }, { 1.f, 2.f, 3.f, 4.f, 5.f } };
    tensor<float, 2> tresult{ shape{ 5, 5 }, { 11, 12, 13, 14, 15, 21, 22, 23, 24, 25, 31, 32, 33,
                                               34, 35, 41, 42, 43, 44, 45, 51, 52, 53, 54, 55 } };

    tensor<float, 2> t3 = t1 + t2;

    CHECK(t3.shape() == shape{ 5, 5 });
    CHECK(t3 == tresult);

    tensor<float, 2> t5 = t4 + t2;
    // tensor<float, 2> t5 = t4 + t2;
    CHECK(t5 == tresult);
}
} // namespace CMT_ARCH_NAME

template <typename T, size_t N1>
struct expression_traits<std::array<T, N1>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 1;

    constexpr static shape<1> get_shape(const std::array<T, N1>& self) { return shape<1>{ N1 }; }
    constexpr static shape<1> get_shape() { return shape<1>{ N1 }; }
};

template <typename T, size_t N1, size_t N2>
struct expression_traits<std::array<std::array<T, N1>, N2>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 2;

    constexpr static shape<2> get_shape(const std::array<std::array<T, N1>, N2>& self)
    {
        return shape<2>{ N2, N1 };
    }
    constexpr static shape<2> get_shape() { return shape<2>{ N2, N1 }; }
};

inline namespace CMT_ARCH_NAME
{

template <typename T, size_t N1, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const std::array<T, N1>& CMT_RESTRICT self, const shape<1>& index,
                                     const axis_params<Axis, N>&)
{
    const T* CMT_RESTRICT const data = self.data();
    return read<N>(data + index[0]);
}

template <typename T, size_t N1, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(std::array<T, N1>& CMT_RESTRICT self, const shape<1>& index,
                                const axis_params<Axis, N>&, const identity<vec<T, N>>& val)
{
    T* CMT_RESTRICT const data = self.data();
    write(data + index[0], val);
}

template <typename T, size_t N1, size_t N2, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const std::array<std::array<T, N1>, N2>& CMT_RESTRICT self,
                                     const shape<2>& index, const axis_params<Axis, N>&)
{
    const T* CMT_RESTRICT const data = self.front().data() + index.front() * N1 + index.back();
    if constexpr (Axis == 1)
    {
        return read<N>(data);
    }
    else
    {
        return gather_stride<N>(data, N1);
    }
}

template <typename T, size_t N1, size_t N2, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(std::array<std::array<T, N1>, N2>& CMT_RESTRICT self, const shape<2>& index,
                                const axis_params<Axis, N>&, const identity<vec<T, N>>& val)
{
    T* CMT_RESTRICT data = self.front().data() + index.front() * N1 + index.back();
    if constexpr (Axis == 1)
    {
        write(data, val);
    }
    else
    {
        scatter_stride(data, val, N1);
    }
}

TEST(tensor_slice)
{
    tensor<double, 3> t1{
        { { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 } },
        { { 9, 10, 11 }, { 12, 13, 14 }, { 15, 16, 17 } },
        { { 18, 19, 20 }, { 21, 22, 23 }, { 24, 25, 26 } },
    };
    CHECK(t1 == trender(truncate(counter(0.0, 9, 3, 1), shape{ 3, 3, 3 })));

    CHECK(trender(slice(t1, shape{ 1, 1, 1 }, shape{ 2, 2, 2 })) ==
          trender(truncate(counter(13.0, 9, 3, 1), shape{ 2, 2, 2 })));
}

TEST(scalars)
{
    CHECK(trender(scalar(3)) == tensor<int, 0>{});
    CHECK(trender(scalar(3)).to_string() == "3");
}

TEST(tensor_lambda)
{
    CHECK(trender(lambda<float, 2>([](shape<2> idx) -> float { return 1 + idx[1] + 3 * idx[0]; }),
                  shape{ 3, 3 }) ==
          tensor<float, 2>{
              { 1, 2, 3 },
              { 4, 5, 6 },
              { 7, 8, 9 },
          });

    CHECK(trender(truncate(lambda<float, 2>([](shape<2> idx) -> float { return 1 + idx[1] + 3 * idx[0]; }),
                           shape{ 3, 3 })) ==
          tensor<float, 2>{
              { 1, 2, 3 },
              { 4, 5, 6 },
              { 7, 8, 9 },
          });
}

TEST(tensor_expressions2)
{
    auto aa = std::array<std::array<double, 2>, 2>{ { { { 1, 2 } }, { { 3, 4 } } } };
    static_assert(expression_traits<decltype(aa)>::dims == 2);
    CHECK(expression_traits<decltype(aa)>::get_shape(aa) == shape{ 2, 2 });
    CHECK(get_elements(aa, { 1, 1 }, axis_params<1, 1>{}) == vec{ 4. });
    CHECK(get_elements(aa, { 1, 0 }, axis_params<1, 2>{}) == vec{ 3., 4. });

    static_assert(expression_traits<decltype(1234.f)>::dims == 0);
    CHECK(expression_traits<decltype(1234.f)>::get_shape(1234.f) == shape{});
    CHECK(get_elements(1234.f, {}, axis_params<0, 3>{}) == vec{ 1234.f, 1234.f, 1234.f });

    process(aa, 123.45f);

    CHECK(aa ==
          std::array<std::array<double, 2>, 2>{ { { { 123.45f, 123.45f } }, { { 123.45f, 123.45f } } } });

    auto a = std::array<double, 2>{ { -5.f, +5.f } };

    process(aa, a);

    CHECK(aa == std::array<std::array<double, 2>, 2>{ { { { -5., +5. } }, { { -5., +5. } } } });
}

TEST(tensor_counter)
{
    std::array<double, 6> x;

    process(x, counter(0.0, 0.5));

    CHECK(x == std::array<double, 6>{ { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5 } });

    std::array<std::array<double, 4>, 3> y;

    process(y, counter(100.0, 1.0, 10.0));

    CHECK(y == std::array<std::array<double, 4>, 3>{ {
                   { { 100.0, 110.0, 120.0, 130.0 } },
                   { { 101.0, 111.0, 121.0, 131.0 } },
                   { { 102.0, 112.0, 122.0, 132.0 } },
               } });
}
namespace tests
{
TEST(tensor_dims)
{
    tensor<double, 6> t12{ shape{ 2, 3, 4, 5, 6, 7 } };

    process(t12, counter(0, 1, 10, 100, 1000, 10000, 100000));

    auto t1 = t12(1, 2, 3, tall(), 5, 6);
    CHECK(render(t1) == univector<double>{ 650321, 651321, 652321, 653321, 654321 });

    CHECK(t12.reduce(std::plus<>{}, 0) == 1648888920);
}
} // namespace tests

TEST(vec_from_cvals)
{
    CHECK(make_vector(csizes<1, 2, 3, 4>) == make_vector<size_t>(1, 2, 3, 4));
    CHECK(make_vector(cconcat(cvalseq<index_t, 2, 0, 0>, cvalseq<index_t, 1, 1>,
                              cvalseq<index_t, 2, 0, 0>)) == make_vector<size_t>(0, 0, 1, 0, 0));
}

TEST(xfunction_test)
{
    auto f = expression_function{ expression_with_arguments{ 3.f, 4.f }, std::plus<>{} };
    float v;
    process(v, f);
    CHECK(v == 7.f);
    static_assert(std::is_same_v<decltype(f), expression_function<std::plus<>, float, float>>);

    auto f2 = expression_function{ expression_with_arguments{ 10.f, std::array{ 1.f, 2.f, 3.f, 4.f, 5.f } },
                                   std::plus<>{} };
    std::array<float, 5> v2;
    process(v2, f2);
    CHECK(v2 == std::array{ 11.f, 12.f, 13.f, 14.f, 15.f });

    auto f3 = scalar(10.f) + std::array{ 1.f, 2.f, 3.f, 4.f, 5.f };
    std::array<float, 5> v3;
    process(v3, f3);
    CHECK(v3 == std::array{ 11.f, 12.f, 13.f, 14.f, 15.f });

    auto f4 = scalar(0) +
              std::array<std::array<float, 1>, 5>{
                  { { { 100.f } }, { { 200.f } }, { { 300.f } }, { { 400.f } }, { { 500.f } } }
              } +
              std::array{ 1.f, 2.f, 3.f, 4.f, 5.f };
    std::array<std::array<float, 5>, 5> v4;

    CHECK(expression_traits<decltype(f4)>::get_shape(f4) == shape{ 5, 5 });
    process(v4, f4);
    CHECK(v4 == std::array<std::array<float, 5>, 5>{ { { { 101.f, 102.f, 103.f, 104.f, 105.f } },
                                                       { { 201.f, 202.f, 203.f, 204.f, 205.f } },
                                                       { { 301.f, 302.f, 303.f, 304.f, 305.f } },
                                                       { { 401.f, 402.f, 403.f, 404.f, 405.f } },
                                                       { { 501.f, 502.f, 503.f, 504.f, 505.f } } } });
}

TEST(xfunction_test2)
{
    CHECK(trender(abs(tensor<float, 2>{ { 1, -2 }, { -1, 3 } })) == tensor<float, 2>{ { 1, 2 }, { 1, 3 } });
    CHECK(trender(min(tensor<float, 2>{ { 1, -2 }, { -1, 3 } }, tensor<float, 2>{ { 0, 3 }, { 2, 1 } })) ==
          tensor<float, 2>{ { 0, -2 }, { -1, 1 } });
}

template <typename Type, index_t Dims>
KFR_FUNCTION expression_counter<Type, Dims> debug_counter(uint64_t scale = 10)
{
    expression_counter<Type, Dims> result;
    result.start = 0;
    uint64_t val = 1;
    for (size_t i = 0; i < Dims; i++)
    {
        result.steps[Dims - 1 - i] = val;
        val *= scale;
    }
    return result;
}

static std::string nl = R"(
)";

TEST(tensor_tostring)
{
    CHECK(as_string(shape{}) == "shape{}");
    CHECK(as_string(shape{ 1, 2, 3 }) == "shape{1, 2, 3}");

    tensor<f32x2, 1> t0(shape<1>{ 3 });
    t0(0) = vec{ 1, 2 };
    t0(1) = vec{ 3, 4 };
    t0(2) = vec{ -1, 1000 };
    CHECK(t0.to_string<fmt_t<f32x2, 'f', 0, 2>>() == "{{1, 2}, {3, 4}, {-1, 1000}}");

    tensor<float, 1> t1(shape<1>{ 60 });
    t1 = debug_counter<float, 1>();
    CHECK(nl + t1.to_string<fmt_t<float, 'f', 2, 0>>(12, 0) + nl == R"(
{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59}
)");

    tensor<float, 2> t2(shape<2>{ 12, 5 });
    t2 = debug_counter<float, 2>();
    CHECK(nl + t2.to_string<fmt_t<float, 'f', 3, 0>>(16, 0) + nl == R"(
{{  0,   1,   2,   3,   4},
 { 10,  11,  12,  13,  14},
 { 20,  21,  22,  23,  24},
 { 30,  31,  32,  33,  34},
 { 40,  41,  42,  43,  44},
 { 50,  51,  52,  53,  54},
 { 60,  61,  62,  63,  64},
 { 70,  71,  72,  73,  74},
 { 80,  81,  82,  83,  84},
 { 90,  91,  92,  93,  94},
 {100, 101, 102, 103, 104},
 {110, 111, 112, 113, 114}}
)");

    tensor<float, 3> t3(shape<3>{ 3, 4, 5 });
    t3 = debug_counter<float, 3>();
    CHECK(nl + t3.to_string<fmt_t<float, 'f', 4, 0>>(16, 0) + nl == R"(
{{{   0,    1,    2,    3,    4},
  {  10,   11,   12,   13,   14},
  {  20,   21,   22,   23,   24},
  {  30,   31,   32,   33,   34}},
 {{ 100,  101,  102,  103,  104},
  { 110,  111,  112,  113,  114},
  { 120,  121,  122,  123,  124},
  { 130,  131,  132,  133,  134}},
 {{ 200,  201,  202,  203,  204},
  { 210,  211,  212,  213,  214},
  { 220,  221,  222,  223,  224},
  { 230,  231,  232,  233,  234}}}
)");

    tensor<float, 4> t4(shape<4>{ 3, 2, 2, 5 });
    t4 = debug_counter<float, 4>();
    CHECK(nl + t4.to_string<fmt_t<float, 'f', 5, 0>>(16, 0) + nl == R"(
{{{{    0,     1,     2,     3,     4},
   {   10,    11,    12,    13,    14}},
  {{  100,   101,   102,   103,   104},
   {  110,   111,   112,   113,   114}}},
 {{{ 1000,  1001,  1002,  1003,  1004},
   { 1010,  1011,  1012,  1013,  1014}},
  {{ 1100,  1101,  1102,  1103,  1104},
   { 1110,  1111,  1112,  1113,  1114}}},
 {{{ 2000,  2001,  2002,  2003,  2004},
   { 2010,  2011,  2012,  2013,  2014}},
  {{ 2100,  2101,  2102,  2103,  2104},
   { 2110,  2111,  2112,  2113,  2114}}}}
)");

    tensor<float, 2> t5(shape<2>{ 10, 1 });
    t5 = debug_counter<float, 2>();
    CHECK(nl + t5.to_string<fmt_t<float, 'f', -1, 0>>(12, 1) + nl == R"(
{{0}, {10}, {20}, {30}, {40}, {50}, {60}, {70}, {80}, {90}}
)");
}

template <typename T, index_t dims1, index_t dims2>
static void test_reshape_body(const tensor<T, dims1>& t1, const tensor<T, dims2>& t2)
{
    CHECK(t1.reshape_may_copy(t2.shape(), true) == t2);

    cforeach(csizeseq<dims2>,
             [&](auto x)
             {
                 constexpr index_t axis = val_of(decltype(x)());
                 ::testo::scope s(
                     as_string("axis = ", axis, " shape = (", t1.shape(), ") -> (", t2.shape(), ")"));
                 CHECK(trender<1, axis>(reshape(t1, t2.shape())) == t2);
                 CHECK(trender<2, axis>(reshape(t1, t2.shape())) == t2);
                 CHECK(trender<4, axis>(reshape(t1, t2.shape())) == t2);
                 CHECK(trender<8, axis>(reshape(t1, t2.shape())) == t2);
             });
}

static void test_reshape() {}

template <typename T, index_t dims1, index_t... dims>
static void test_reshape(const tensor<T, dims1>& t1, const tensor<T, dims>&... ts)
{
    cforeach(std::make_tuple((&ts)...),
             [&](auto t2)
             {
                 test_reshape_body(t1, *t2);
                 test_reshape_body(*t2, t1);
             });

    test_reshape(ts...);
}

TEST(expression_reshape)
{
    std::array<float, 12> x;
    process(reshape(x, shape{ 3, 4 }), expression_counter<float, 2>{ 0, { 10, 1 } });
    CHECK(x == std::array<float, 12>{ { 0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23 } });

    test_reshape(tensor<float, 1>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, //
                 tensor<float, 2>{ { 0, 1, 2, 3, 4, 5 }, { 6, 7, 8, 9, 10, 11 } },
                 tensor<float, 2>{ { 0, 1, 2, 3 }, { 4, 5, 6, 7 }, { 8, 9, 10, 11 } },
                 tensor<float, 2>{ { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 }, { 9, 10, 11 } },
                 tensor<float, 2>{ { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 8, 9 }, { 10, 11 } },
                 tensor<float, 2>{
                     { 0 }, { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 }, { 8 }, { 9 }, { 10 }, { 11 } });

    test_reshape(tensor<float, 1>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, //
                 tensor<float, 3>{ { { 0, 1 }, { 2, 3 }, { 4, 5 } }, { { 6, 7 }, { 8, 9 }, { 10, 11 } } },
                 tensor<float, 4>{ { { { 0 }, { 1 } }, { { 2 }, { 3 } }, { { 4 }, { 5 } } },
                                   { { { 6 }, { 7 } }, { { 8 }, { 9 } }, { { 10 }, { 11 } } } });

    test_reshape(
        tensor<float, 1>{
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23 }, //
        tensor<float, 3>{ { { 0, 1 }, { 2, 3 }, { 4, 5 } },
                          { { 6, 7 }, { 8, 9 }, { 10, 11 } },
                          { { 12, 13 }, { 14, 15 }, { 16, 17 } },
                          { { 18, 19 }, { 20, 21 }, { 22, 23 } } },
        tensor<float, 4>{
            { { { 0, 1 }, { 2, 3 } }, { { 4, 5 }, { 6, 7 } }, { { 8, 9 }, { 10, 11 } } },
            { { { 12, 13 }, { 14, 15 } }, { { 16, 17 }, { 18, 19 } }, { { 20, 21 }, { 22, 23 } } } });
}

} // namespace CMT_ARCH_NAME

#if 0
shape<4> sh{ 2, 3, 4, 5 };

extern "C" __declspec(dllexport) bool assembly_test1(shape<4>& x)
{
    return kfr::internal_generic::increment_indices(x, shape<4>(0), sh);
}

extern "C" __declspec(dllexport) bool assembly_test2(std::array<std::array<double, 2>, 2>& aa,
                                                     std::array<double, 2>& a)
{
    return process(aa, a).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test3(std::array<double, 16>& x)
{
    return process(x, 0.5).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test4(std::array<double, 16>& x)
{
    return process(x, counter(1000.0, 1.0)).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test5(const tensor<double, 3>& x)
{
    return process(x, counter(1000.0, 1.0, 2.0, 3.0)).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test6(const tensor<double, 2>& x)
{
    return process(x, counter(1000.0, 1.0, 2.0)).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test7(const tensor<double, 2>& x)
{
    return process(x, 12345.).front() > 0;
}

extern "C" __declspec(dllexport) index_t assembly_test8_2(const shape<2>& x, const shape<2>& y)
{
    return x.dot(y);
}

extern "C" __declspec(dllexport) index_t assembly_test8_4(const shape<4>& x, const shape<4>& y)
{
    return x.dot(y);
}

extern "C" __declspec(dllexport) void assembly_test9(int64_t* dst, size_t stride)
{
    scatter_stride(dst, enumerate(vec_shape<int64_t, 8>()), stride);
}
constexpr inline index_t rank = 1;
extern "C" __declspec(dllexport) void assembly_test10(tensor<double, rank>& t12,
                                                      const expression_counter<double, rank>& ctr)
{
    process(t12, ctr);
}
extern "C" __declspec(dllexport) void assembly_test11(f64x2& x, u64x2 y) { x = y; }

extern "C" __declspec(dllexport) void assembly_test12(
    std::array<std::array<uint32_t, 4>, 4>& x,
    const expression_function<std::plus<>, std::array<std::array<uint32_t, 1>, 4>&,
                              std::array<std::array<uint32_t, 4>, 1>&>& y)
{
    process(x, y);
}

extern "C" __declspec(dllexport) void assembly_test13(const tensor<float, 1>& x, const tensor<float, 1>& y)
{
    process(x, y * 0.5f);
}

template <typename T, size_t N1, size_t N2>
using array2d = std::array<std::array<T, N2>, N1>;

extern "C" __declspec(dllexport) void assembly_test14(std::array<float, 32>& x,
                                                      const std::array<float, 32>& y)
{
    process(x, reverse(y));
}

extern "C" __declspec(dllexport) void assembly_test15(array2d<float, 32, 32>& x,
                                                      const array2d<float, 32, 32>& y)
{
    process(x, reverse(y));
}

extern "C" __declspec(dllexport) void assembly_test16a(array2d<double, 8, 2>& x,
                                                       const array2d<double, 8, 2>& y)
{
    process<8, 0>(x, y * y);
}
extern "C" __declspec(dllexport) void assembly_test16b(array2d<double, 8, 2>& x,
                                                       const array2d<double, 8, 2>& y)
{
    process<2, 1>(x, y * y);
}

extern "C" __declspec(dllexport) void assembly_test17a(const tensor<double, 2>& x, const tensor<double, 2>& y)
{
    expression_function ysqr = expression_function{ expression_with_arguments{ y }, fn::sqr{} };
    process<8, 0>(x, ysqr);
}
extern "C" __declspec(dllexport) void assembly_test17b(const tensor<double, 2>& x, const tensor<double, 2>& y)
{
    expression_function ysqr = expression_function{ expression_with_arguments{ y }, fn::sqr{} };
    process<2, 1>(x, ysqr);
}

extern "C" __declspec(dllexport) void assembly_test18a(const tensor<double, 2>& x, const tensor<double, 2>& y)
{
    expression_function ysqr = expression_function{ expression_with_arguments{ y }, fn::sqr{} };
    process<8, 0>(fixshape(x, fixed_shape<8, 2>), fixshape(ysqr, fixed_shape<8, 2>));
}
extern "C" __declspec(dllexport) void assembly_test18b(const tensor<double, 2>& x, const tensor<double, 2>& y)
{
    expression_function ysqr = expression_function{ expression_with_arguments{ y }, fn::sqr{} };
    process<2, 1>(fixshape(x, fixed_shape<8, 2>), fixshape(ysqr, fixed_shape<8, 2>));
}

extern "C" __declspec(dllexport) void assembly_test19(const tensor<double, 2>& x,
                                                      const expression_reshape<tensor<double, 2>, 2>& y)
{
    process(x, y);
}

extern "C" __declspec(dllexport) shape<2> assembly_test20_2(const shape<2>& x, size_t fl)
{
    return x.from_flat(fl);
}
extern "C" __declspec(dllexport) shape<4> assembly_test20_4(const shape<4>& x, size_t fl)
{
    return x.from_flat(fl);
}

extern "C" __declspec(dllexport) shape<4> assembly_test21(const shape<4>& x, size_t fl)
{
    return x.from_flat(fl);
}
extern "C" __declspec(dllexport) float assembly_test22(const std::array<float, 440>& x,
                                                       const std::array<float, 440>& y)
{
    return dotproduct(x, y);
}
extern "C" __declspec(dllexport) float assembly_test23(const std::array<float, 440>& x) { return rms(x); }
#endif

struct val
{
};
template <>
struct expression_traits<val> : expression_traits_defaults
{
    using value_type             = float;
    constexpr static size_t dims = 0;
    constexpr static shape<dims> get_shape(const val&) { return {}; }
    constexpr static shape<dims> get_shape() { return {}; }
};

inline namespace CMT_ARCH_NAME
{
val rvint_func() { return val{}; }
val& lvint_func()
{
    static val v;
    return v;
}
TEST(expression_with_arguments)
{
    expression_function fn1 = expression_function{ expression_with_arguments{ rvint_func() }, fn::add{} };
    static_assert(std::is_same_v<decltype(fn1)::nth<0>, val>);

    expression_function fn2 = expression_function{ expression_with_arguments{ lvint_func() }, fn::add{} };
    static_assert(std::is_same_v<decltype(fn2)::nth<0>, val&>);

    expression_function fn3 =
        expression_function{ expression_with_arguments{ std::as_const(lvint_func()) }, fn::add{} };
    static_assert(std::is_same_v<decltype(fn3)::nth<0>, const val&>);
}

TEST(slices)
{
    const auto _ = std::nullopt;
    tensor<float, 1> t1{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    CHECK(t1(tstart(3)) == tensor<float, 1>{ 3, 4, 5, 6, 7, 8, 9 });
    CHECK(t1(tstop(3)) == tensor<float, 1>{ 0, 1, 2 });
    CHECK(t1(trange(3, 7)) == tensor<float, 1>{ 3, 4, 5, 6 });

    CHECK(t1(tstart(10)) == tensor<float, 1>{});
    CHECK(t1(tstop(0)) == tensor<float, 1>{});
    CHECK(t1(trange(7, 3)) == tensor<float, 1>{});

    CHECK(t1(tstart(-2)) == tensor<float, 1>{ 8, 9 });
    CHECK(t1(trange(-7, -4)) == tensor<float, 1>{ 3, 4, 5 });
    CHECK(t1(tall()) == tensor<float, 1>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });

    CHECK(t1(trange(3, _)) == tensor<float, 1>{ 3, 4, 5, 6, 7, 8, 9 });
    CHECK(t1(trange(_, 7)) == tensor<float, 1>{ 0, 1, 2, 3, 4, 5, 6 });

    CHECK(t1(trange(_, _, 2)) == tensor<float, 1>{ 0, 2, 4, 6, 8 });
    CHECK(t1(trange(_, _, 5)) == tensor<float, 1>{ 0, 5 });
    CHECK(t1(trange(_, _, 12)) == tensor<float, 1>{ 0 });
    CHECK(t1(trange(1, _, 2)) == tensor<float, 1>{ 1, 3, 5, 7, 9 });
    CHECK(t1(trange(1, _, 5)) == tensor<float, 1>{ 1, 6 });
    CHECK(t1(trange(1, _, 12)) == tensor<float, 1>{ 1 });

    CHECK(t1(tstep(2))(tstep(2)) == tensor<float, 1>{ 0, 4, 8 });
    CHECK(t1(tstep(2))(tstep(2))(tstep(2)) == tensor<float, 1>{ 0, 8 });
    CHECK(t1(tstep(2))(tstep(3)) == tensor<float, 1>{ 0, 6 });

    CHECK(t1(trange(_, _, -1)) == tensor<float, 1>{ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 });
    CHECK(t1(trange(5, _, -1)) == tensor<float, 1>{ 5, 4, 3, 2, 1, 0 });
    CHECK(t1(trange(1, 0, -1)) == tensor<float, 1>{ 1 });

    CHECK(t1(trange(3, 3 + 12, 0)) == tensor<float, 1>{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 });
}

TEST(complex_tensors)
{
    tensor<complex<float>, 1> t1{
        complex<float>(0, -1),
    };
    CHECK(trender(expression_function{ expression_with_arguments{ t1, complex<float>(0, 1) }, fn::mul{} }) ==
          tensor<complex<float>, 1>{ complex<float>(1, 0) });
    CHECK(trender(expression_function{ expression_with_arguments{ t1, complex<float>(1, 0) }, fn::mul{} }) ==
          tensor<complex<float>, 1>{ complex<float>(0, -1) });
    CHECK(trender(expression_function{ expression_with_arguments{ t1, complex<float>(0, -1) }, fn::mul{} }) ==
          tensor<complex<float>, 1>{ complex<float>(-1, 0) });
    CHECK(trender(expression_function{ expression_with_arguments{ t1, complex<float>(-1, 0) }, fn::mul{} }) ==
          tensor<complex<float>, 1>{ complex<float>(0, 1) });
}

TEST(from_ilist)
{
    tensor<float, 1> t1{ 10, 20, 30, 40 };
    CHECK(t1 == tensor<float, 1>(shape{ 4 }, { 10, 20, 30, 40 }));

    tensor<float, 2> t2{ { 10, 20 }, { 30, 40 } };
    CHECK(t2 == tensor<float, 2>(shape{ 2, 2 }, { 10, 20, 30, 40 }));

    tensor<float, 2> t3{ { 10, 20 } };
    CHECK(t3 == tensor<float, 2>(shape{ 1, 2 }, { 10, 20 }));

    tensor<float, 3> t4{ { { 10, 20 }, { 30, 40 } }, { { 50, 60 }, { 70, 80 } } };
    CHECK(t4 == tensor<float, 3>(shape{ 2, 2, 2 }, { 10, 20, 30, 40, 50, 60, 70, 80 }));
}

TEST(sharing_data)
{
    tensor<int, 2> t{ { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };
    auto t2  = t; // share data
    t2(0, 0) = 10;
    CHECK(t == tensor<int, 2>{ { 10, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } });
    auto t3 = t(0, tall());
    CHECK(t3 == tensor<int, 1>{ 10, 2, 3 });
    t3 *= 10;
    CHECK(t3 == tensor<int, 1>{ 100, 20, 30 });
    CHECK(t == tensor<int, 2>{ { 100, 20, 30 }, { 4, 5, 6 }, { 7, 8, 9 } });
    t(trange(0, 2), trange(0, 2)) = 0;
    CHECK(t == tensor<int, 2>{ { 0, 0, 30 }, { 0, 0, 6 }, { 7, 8, 9 } });
}

TEST(tensor_from_container)
{
    std::vector<int> a{ 1, 2, 3 };
    auto t = tensor_from_container(a);
    CHECK(t.shape() == shape{ 3 });
    CHECK(t == tensor<int, 1>{ 1, 2, 3 });
}

} // namespace CMT_ARCH_NAME

template <typename T, index_t Size>
struct identity_matrix
{
};

template <typename T, index_t Size>
struct expression_traits<identity_matrix<T, Size>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 2;
    constexpr static shape<2> get_shape(const identity_matrix<T, Size>& self) { return { Size, Size }; }
    constexpr static shape<2> get_shape() { return { Size, Size }; }
};

template <typename T, index_t Size, index_t Axis, size_t N>
vec<T, N> get_elements(const identity_matrix<T, Size>& self, const shape<2>& index,
                       const axis_params<Axis, N>& sh)
{
    return select(indices<0>(index, sh) == indices<1>(index, sh), 1, 0);
}

inline namespace CMT_ARCH_NAME
{

TEST(identity_matrix)
{
    CHECK(trender(identity_matrix<float, 3>{}) == tensor<float, 2>{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } });
}

} // namespace CMT_ARCH_NAME

} // namespace kfr
CMT_PRAGMA_MSVC(warning(pop))
