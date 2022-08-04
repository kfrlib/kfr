/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/base/new_expressions.hpp>
#include <kfr/base/tensor.hpp>
#include <kfr/io/tostring.hpp>

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 5051))

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(vec_deduction)
{
    vec v{ 1, 2, 3 };
    static_assert(std::is_same_v<decltype(v), vec<int, 3>>);

    tensor<float, 2> t2{ shape{ 20, 40 } };
}

TEST(shape)
{
    using internal_generic::increment_indices_return;
    using internal_generic::null_index;
    CHECK(size_of_shape(shape{ 4, 3 }) == 12);
    CHECK(size_of_shape(shape{ 1 }) == 1);

    CHECK(internal_generic::strides_for_shape(shape{ 2, 3, 4 }) == shape{ 12, 4, 1 });

    CHECK(internal_generic::strides_for_shape(shape{ 2, 3, 4 }, 10) == shape{ 120, 40, 10 });

    CHECK(increment_indices_return(shape{ 0, 0, 0 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) ==
          shape{ 0, 0, 1 });
    CHECK(increment_indices_return(shape{ 0, 0, 3 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) ==
          shape{ 0, 1, 0 });
    CHECK(increment_indices_return(shape{ 0, 2, 0 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) ==
          shape{ 0, 2, 1 });
    CHECK(increment_indices_return(shape{ 0, 2, 3 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) ==
          shape{ 1, 0, 0 });
    CHECK(increment_indices_return(shape{ 1, 2, 3 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) ==
          shape{ null_index, null_index, null_index });

    CHECK(shape{ 3, 4, 5 }.to_flat(shape{ 0, 0, 0 }) == 0);
    CHECK(shape{ 3, 4, 5 }.to_flat(shape{ 2, 3, 4 }) == 59);

    CHECK(shape{ 3, 4, 5 }.from_flat(0) == shape{ 0, 0, 0 });
    CHECK(shape{ 3, 4, 5 }.from_flat(59) == shape{ 2, 3, 4 });
}

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

TEST(tensor_expression_assign)
{
    tensor<float, 1> t1{ shape{ 32 }, 0.f };

    t1 = counter();

    CHECK(t1.size() == 32);
    CHECK(t1(0) == 0.f);
    CHECK(t1(1) == 1.f);
    CHECK(t1(31) == 31.f);
}

DTEST(tensor_expression)
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
    CHECK(sum(t4) == 36);

    t4(trange(2, 4), trange(2, 4)) = scalar(10);

    CHECK(t4(0, 0) == 1.f);
    CHECK(t4(1, 1) == 1.f);
    CHECK(t4(2, 2) == 10.f);
    CHECK(t4(3, 3) == 10.f);
    CHECK(t4(5, 5) == 1.f);
    CHECK(sum(t4) == 72);

    t4(trange(2, 4), trange(2, 4)) = 10 + counter();

    CHECK(t4(2, 2) == 10.f);
    CHECK(t4(2, 3) == 11.f);
    CHECK(t4(3, 2) == 12.f);
    CHECK(t4(3, 3) == 13.f);
}

template <typename T, index_t dims1, index_t dims2, index_t outdims = const_max(dims1, dims2)>
KFR_INTRINSIC tensor<T, outdims> operator+(const tensor<T, dims1>& x, const tensor<T, dims2>& y)
{
    return tapply(x, y, fn::add{});
}
template <typename T, index_t dims1, index_t dims2, index_t outdims = const_max(dims1, dims2)>
KFR_INTRINSIC tensor<T, outdims> operator-(const tensor<T, dims1>& x, const tensor<T, dims2>& y)
{
    return tapply(x, y, fn::sub{});
}
template <typename T, index_t dims1, index_t dims2, index_t outdims = const_max(dims1, dims2)>
KFR_INTRINSIC tensor<T, outdims> operator*(const tensor<T, dims1>& x, const tensor<T, dims2>& y)
{
    return tapply(x, y, fn::mul{});
}
template <typename T, index_t dims1, index_t dims2, index_t outdims = const_max(dims1, dims2)>
KFR_INTRINSIC tensor<T, outdims> operator/(const tensor<T, dims1>& x, const tensor<T, dims2>& y)
{
    return tapply(x, y, fn::div{});
}

TEST(tensor_broadcast)
{
    using internal_generic::can_assign_from;
    using internal_generic::common_shape;
    using internal_generic::same_layout;

    CHECK(common_shape(shape{ 1, 5 }, shape{ 5, 1 }) == shape{ 5, 5 });
    CHECK(common_shape(shape{ 5 }, shape{ 5, 1 }) == shape{ 5, 5 });
    CHECK(common_shape(shape{ 1, 1, 1 }, shape{ 2, 5, 1 }) == shape{ 2, 5, 1 });
    CHECK(common_shape(shape{ 1 }, shape{ 2, 5, 7 }) == shape{ 2, 5, 7 });

    CHECK(can_assign_from(shape{ 1, 4 }, shape{ 1, 4 }));
    CHECK(!can_assign_from(shape{ 1, 4 }, shape{ 4, 1 }));
    CHECK(can_assign_from(shape{ 1, 4 }, shape{ 1, 1 }));
    CHECK(can_assign_from(shape{ 1, 4 }, shape{ 1 }));
    CHECK(can_assign_from(shape{ 1, 4 }, shape{}));

    tensor<float, 2> t1{ shape{ 1, 5 }, { 1.f, 2.f, 3.f, 4.f, 5.f } };
    tensor<float, 2> t2{ shape{ 5, 1 }, { 10.f, 20.f, 30.f, 40.f, 50.f } };
    tensor<float, 1> t4{ shape{ 5 }, { 1.f, 2.f, 3.f, 4.f, 5.f } };
    tensor<float, 2> tresult{ shape{ 5, 5 }, { 11, 12, 13, 14, 15, 21, 22, 23, 24, 25, 31, 32, 33,
                                                 34, 35, 41, 42, 43, 44, 45, 51, 52, 53, 54, 55 } };

    tensor<float, 2> t3 = tapply(t1, t2, fn::add{});

    CHECK(t3.shape() == shape{ 5, 5 });
    CHECK(t3 == tresult);

    tensor<float, 2> t5 = tapply(t4, t2, fn::add{});
    // tensor<float, 2> t5 = t4 + t2;
    CHECK(t5 == tresult);

    CHECK(same_layout(shape{ 2, 3, 4 }, shape{ 2, 3, 4 }));
    CHECK(same_layout(shape{ 1, 2, 3, 4 }, shape{ 2, 3, 4 }));
    CHECK(same_layout(shape{ 2, 3, 4 }, shape{ 2, 1, 1, 3, 4 }));
    CHECK(same_layout(shape{ 2, 3, 4 }, shape{ 2, 3, 4, 1 }));
    CHECK(same_layout(shape{ 2, 1, 3, 4 }, shape{ 1, 2, 3, 4, 1 }));

    CHECK(!same_layout(shape{ 2, 1, 3, 4 }, shape{ 1, 2, 4, 3, 1 }));
    CHECK(!same_layout(shape{ 2, 1, 3, 4 }, shape{ 1, 2, 4, 3, 0 }));
}
} // namespace CMT_ARCH_NAME

template <typename T, index_t Dims = 1>
struct tcounter
{
    T start;
    std::array<T, Dims> steps;
};

template <typename T, index_t Dims>
struct expression_traits<tcounter<T, Dims>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;

    constexpr static shape<dims> shapeof(const tcounter<T, Dims>& self) { return max_index_t; }
    constexpr static shape<dims> shapeof() { return max_index_t; }
};

template <typename T, size_t N1>
struct expression_traits<std::array<T, N1>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 1;

    constexpr static shape<1> shapeof(const std::array<T, N1>& self) { return { N1 }; }
    constexpr static shape<1> shapeof() { return { N1 }; }
};

template <typename T, size_t N1, size_t N2>
struct expression_traits<std::array<std::array<T, N1>, N2>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 2;

    constexpr static shape<2> shapeof(const std::array<std::array<T, N1>, N2>& self) { return { N2, N1 }; }
    constexpr static shape<2> shapeof() { return { N2, N1 }; }
};

inline namespace CMT_ARCH_NAME
{
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const tcounter<T, 1>& self, const shape<1>& index, csize_t<N> sh)
{
    T acc = self.start;
    acc += static_cast<T>(index.front()) * self.steps.front();
    return acc + enumerate(vec_shape<T, N>(), self.steps.back());
}
template <typename T, index_t dims, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const tcounter<T, dims>& self, const shape<dims>& index, csize_t<N> sh)
{
    T acc                 = self.start;
    vec<T, dims> tindices = cast<T>(*index);
    cfor(csize<0>, csize<dims>, [&](auto i) CMT_INLINE_LAMBDA { acc += tindices[i] * self.steps[i]; });
    return acc + enumerate(vec_shape<T, N>(), self.steps.back());
}

template <typename T, size_t N1, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const std::array<T, N1>& CMT_RESTRICT self, const shape<1>& index,
                                     csize_t<N> sh)
{
    const T* CMT_RESTRICT const data = self.data();
    return read<N>(data + std::min(index[0], static_cast<index_t>(N1 - 1)));
}

template <typename T, size_t N1, size_t N>
KFR_INTRINSIC void set_elements(std::array<T, N1>& CMT_RESTRICT self, const shape<1>& index, csize_t<N>,
                                const identity<vec<T, N>>& val)
{
    T* CMT_RESTRICT const data = self.data();
    write(data + std::min(index[0], static_cast<index_t>(N1 - 1)), val);
}

template <typename T, size_t N1, size_t N2, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const std::array<std::array<T, N1>, N2>& CMT_RESTRICT self,
                                     const shape<2>& index, csize_t<N> sh)
{
    const T* CMT_RESTRICT const data = self[std::min(index[0], static_cast<index_t>(N2 - 1))].data();
    return read<N>(data + std::min(index[1], static_cast<index_t>(N1 - 1)));
}

template <typename T, size_t N1, size_t N2, size_t N>
KFR_INTRINSIC void set_elements(std::array<std::array<T, N1>, N2>& CMT_RESTRICT self, const shape<2>& index,
                                csize_t<N>, const identity<vec<T, N>>& val)
{
    T* CMT_RESTRICT const data = self[std::min(index[0], static_cast<index_t>(N2 - 1))].data();
    write(data + std::min(index[1], static_cast<index_t>(N1 - 1)), val);
}

TEST(tensor_expressions2)
{
    auto aa = std::array<std::array<double, 2>, 2>{ { { { 1, 2 } }, { { 3, 4 } } } };
    static_assert(expression_traits<decltype(aa)>::dims == 2);
    CHECK(expression_traits<decltype(aa)>::shapeof(aa) == shape{ 2, 2 });
    CHECK(get_elements(aa, { 1, 1 }, csize_t<1>{}) == vec{ 4. });
    CHECK(get_elements(aa, { 1, 0 }, csize_t<2>{}) == vec{ 3., 4. });

    static_assert(expression_traits<decltype(1234.f)>::dims == 0);
    CHECK(expression_traits<decltype(1234.f)>::shapeof(1234.f) == shape{});
    CHECK(get_elements(1234.f, {}, csize_t<3>{}) == vec{ 1234.f, 1234.f, 1234.f });

    tprocess(aa, 123.45f);

    CHECK(aa ==
          std::array<std::array<double, 2>, 2>{ { { { 123.45f, 123.45f } }, { { 123.45f, 123.45f } } } });

    auto a = std::array<double, 2>{ { -5.f, +5.f } };

    tprocess(aa, a);

    CHECK(aa == std::array<std::array<double, 2>, 2>{ { { { -5., +5. } }, { { -5., +5. } } } });
}

TEST(tensor_counter)
{
    std::array<double, 6> x;

    tprocess(x, tcounter<double>{ 0.0, { { 0.5 } } });

    CHECK(x == std::array<double, 6>{ { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5 } });

    std::array<std::array<double, 4>, 3> y;

    tprocess(y, tcounter<double, 2>{ 100.0, { { 1.0, 10.0 } } });

    CHECK(y == std::array<std::array<double, 4>, 3>{ {
                   { { 100.0, 110.0, 120.0, 130.0 } },
                   { { 101.0, 111.0, 121.0, 131.0 } },
                   { { 102.0, 112.0, 122.0, 132.0 } },
               } });
}

DTEST(tensor_dims)
{
    tensor<double, 6> t12{ shape{ 2, 3, 4, 5, 6, 7 } };

    tprocess(t12, tcounter<double, 6>{ 0, { { 1, 10, 100, 1000, 10000, 100000 } } });

    auto t1 = t12(1, 2, 3, tall(), 5, 6);
    CHECK(render(t1) == univector<double>{ 650321, 651321, 652321, 653321, 654321 });

    CHECK(t12.reduce(std::plus<>{}, 0) == 1648888920);
}

TEST(vec_from_cvals)
{
    CHECK(make_vector(csizes<1, 2, 3, 4>) == make_vector<size_t>(1, 2, 3, 4));
    CHECK(make_vector(cconcat(cvalseq<index_t, 2, 0, 0>, cvalseq<index_t, 1, 1>,
                              cvalseq<index_t, 2, 0, 0>)) == make_vector<size_t>(0, 0, 1, 0, 0));
}

template <typename X1, typename X2, KFR_ACCEPT_EXPRESSIONS(X1, X2)>
KFR_FUNCTION xfunction<fn::add, X1, X2> operator+(X1&& x1, X2&& x2)
{
    return { xwitharguments{ std::forward<X1>(x1), std::forward<X2>(x2) }, fn::add{} };
}

template <typename X1, typename X2, KFR_ACCEPT_EXPRESSIONS(X1, X2)>
KFR_FUNCTION xfunction<fn::mul, X1, X2> operator*(X1&& x1, X2&& x2)
{
    return { xwitharguments{ std::forward<X1>(x1), std::forward<X2>(x2) }, fn::mul{} };
}

// template <typename X1>
// KFR_FUNCTION xfunction<fn::neg, X1> operator-(X1&& x1)
// {
//     return { xwitharguments{ std::forward<X1>(x1) }, fn::neg{} };
// }

TEST(xfunction_test)
{
    auto f = xfunction{ xwitharguments{ 3.f, 4.f }, std::plus<>{} };
    float v;
    tprocess(v, f);
    CHECK(v == 7.f);
    static_assert(std::is_same_v<decltype(f), xfunction<std::plus<>, float, float>>);

    auto f2 = xfunction{ xwitharguments{ 10.f, std::array{ 1.f, 2.f, 3.f, 4.f, 5.f } }, std::plus<>{} };
    std::array<float, 5> v2;
    tprocess(v2, f2);
    CHECK(v2 == std::array{ 11.f, 12.f, 13.f, 14.f, 15.f });

    auto f3 = 10.f + std::array{ 1.f, 2.f, 3.f, 4.f, 5.f };
    std::array<float, 5> v3;
    tprocess(v3, f3);
    CHECK(v3 == std::array{ 11.f, 12.f, 13.f, 14.f, 15.f });

    auto f4 = std::array<std::array<float, 1>, 5>{
        { { { 100.f } }, { { 200.f } }, { { 300.f } }, { { 400.f } }, { { 500.f } } }
    } + std::array{ 1.f, 2.f, 3.f, 4.f, 5.f };
    std::array<std::array<float, 5>, 5> v4;

    CHECK(expression_traits<decltype(f4)>::shapeof(f4) == shape{ 5, 5 });
    tprocess(v4, f4);
    CHECK(v4 == std::array<std::array<float, 5>, 5>{ { { { 101.f, 102.f, 103.f, 104.f, 105.f } },
                                                       { { 201.f, 202.f, 203.f, 204.f, 205.f } },
                                                       { { 301.f, 302.f, 303.f, 304.f, 305.f } },
                                                       { { 401.f, 402.f, 403.f, 404.f, 405.f } },
                                                       { { 501.f, 502.f, 503.f, 504.f, 505.f } } } });
}

} // namespace CMT_ARCH_NAME

#ifdef _MSC_VER
shape<4> sh{ 2, 3, 4, 5 };

extern "C" __declspec(dllexport) bool assembly_test1(shape<4>& x)
{
    return kfr::internal_generic::increment_indices(x, shape<4>(0), sh);
}

extern "C" __declspec(dllexport) bool assembly_test2(std::array<std::array<double, 2>, 2>& aa,
                                                     std::array<double, 2>& a)
{
    return tprocess(aa, a).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test3(std::array<double, 16>& x)
{
    return tprocess(x, 0.5).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test4(std::array<double, 16>& x)
{
    return tprocess(x, tcounter<double>{ 1000.0, { { 1.0 } } }).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test5(const tensor<double, 3>& x)
{
    return tprocess(x, tcounter<double, 3>{ 1000.0, { { 1.0, 2.0, 3.0 } } }).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test6(const tensor<double, 2>& x)
{
    return tprocess(x, tcounter<double, 2>{ 1000.0, { { 1.0, 2.0 } } }).front() > 0;
}

extern "C" __declspec(dllexport) bool assembly_test7(const tensor<double, 2>& x)
{
    return tprocess(x, 12345.).front() > 0;
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
                                                      const tcounter<double, rank>& ctr)
{
    tprocess(t12, ctr);
}
extern "C" __declspec(dllexport) void assembly_test11(f64x2& x, u64x2 y) { x = y; }

extern "C" __declspec(dllexport) void assembly_test12(
    std::array<std::array<uint32_t, 4>, 4>& x,
    const xfunction<std::plus<>, std::array<std::array<uint32_t, 1>, 4>&,
                    std::array<std::array<uint32_t, 4>, 1>&>& y)
{
    // [[maybe_unused]] constexpr auto sh1 = expression_traits<decltype(x)>::shapeof();
    // [[maybe_unused]] constexpr auto sh2 = expression_traits<decltype(y)>::shapeof();

    // static_assert(sh1 == shape{ 4, 4 });
    // static_assert(sh2 == shape{ 4, 4 });
    tprocess(x, y);
}

extern "C" __declspec(dllexport) void assembly_test13(const tensor<float, 1>& x, const tensor<float, 1>& y)
{
    // [[maybe_unused]] constexpr auto sh1 = expression_traits<decltype(x)>::shapeof();
    // [[maybe_unused]] constexpr auto sh2 = expression_traits<decltype(y)>::shapeof();

    // static_assert(sh1 == shape{ 4, 4 });
    // static_assert(sh2 == shape{ 4, 4 });
    tprocess(x, y * 0.5f);
}
#endif

struct val
{
};
template <>
struct expression_traits<val> : expression_traits_defaults
{
    using value_type             = float;
    constexpr static size_t dims = 0;
    constexpr static shape<dims> shapeof(const val&) { return {}; }
    constexpr static shape<dims> shapeof() { return {}; }
};

inline namespace CMT_ARCH_NAME
{
val rvint_func() { return val{}; }
val& lvint_func()
{
    static val v;
    return v;
}
TEST(xwitharguments)
{
    xfunction fn1 = xfunction{ xwitharguments{ rvint_func() }, fn::add{} };
    static_assert(std::is_same_v<decltype(fn1)::nth<0>, val>);

    xfunction fn2 = xfunction{ xwitharguments{ lvint_func() }, fn::add{} };
    static_assert(std::is_same_v<decltype(fn2)::nth<0>, val&>);

    xfunction fn3 = xfunction{ xwitharguments{ std::as_const(lvint_func()) }, fn::add{} };
    static_assert(std::is_same_v<decltype(fn3)::nth<0>, const val&>);
}

TEST(enumerate)
{
    CHECK(enumerate(vec_shape<int, 4>{}, 4) == vec{ 0, 4, 8, 12 });
    CHECK(enumerate(vec_shape<int, 8>{}, 3) == vec{ 0, 3, 6, 9, 12, 15, 18, 21 });
    CHECK(enumerate(vec_shape<int, 7>{}, 3) == vec{ 0, 3, 6, 9, 12, 15, 18 });
}
} // namespace CMT_ARCH_NAME

} // namespace kfr
CMT_PRAGMA_MSVC(warning(pop))
