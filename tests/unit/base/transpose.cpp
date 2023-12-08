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
CMT_PRAGMA_MSVC(warning(disable : 4244))

namespace kfr
{

inline namespace CMT_ARCH_NAME
{
template <typename T, bool Transposed = false>
struct expression_test_matrix : public expression_traits_defaults
{
    shape<2> matrix_shape;
    index_t mark;
    expression_test_matrix(index_t rows, size_t cols, index_t mark = 10000)
        : matrix_shape{ rows, cols }, mark(mark)
    {
        if constexpr (Transposed)
            std::swap(matrix_shape[0], matrix_shape[1]);
    }

    using value_type             = T;
    constexpr static size_t dims = 2;
    constexpr static shape<2> get_shape(const expression_test_matrix& self) { return self.matrix_shape; }
    constexpr static shape<2> get_shape() { return {}; }

    template <index_t Axis, size_t N>
    friend vec<T, N> get_elements(const expression_test_matrix& self, shape<2> index,
                                  const axis_params<Axis, N>&)
    {
        shape<2> scale{ self.mark, 1 };
        if constexpr (Transposed)
            std::swap(scale[0], scale[1]);
        vec<T, N> result;
        for (size_t i = 0; i < N; ++i)
        {
            result[i] = index[0] * scale[0] + index[1] * scale[1];
            index[Axis] += 1;
        }
        return result;
    }
};

template <typename T>
static void test_transpose(size_t rows, size_t cols, size_t mark = 10000)
{
    tensor<T, 2> t = expression_test_matrix<T>(rows, cols, mark);

    tensor<T, 2> t2(shape<2>{ cols, rows });
    univector<T> tt(t.size());
    auto d  = tensor<T, 2>(tt.data(), shape{ rows, cols }, nullptr);
    auto d2 = tensor<T, 2>(tt.data(), shape{ cols, rows }, nullptr);
    CHECK(d.data() == d2.data());
    d  = expression_test_matrix<T>(rows, cols, mark);
    t2 = -1;
    matrix_transpose(t2.data(), t.data(), shape{ rows, cols });

    matrix_transpose(d2.data(), d.data(), shape{ rows, cols });

    testo::scope s(as_string("type=", type_name<T>(), " rows=", rows, " cols=", cols));

    auto erro = maxof(cabs(t2 - expression_test_matrix<T, true>(rows, cols, mark)));
    CHECK(erro == 0);

    auto erri = maxof(cabs(d2 - expression_test_matrix<T, true>(rows, cols, mark)));
    CHECK(erri == 0);
}

[[maybe_unused]] static void test_transpose_t(size_t rows, size_t cols, size_t mark = 10000)
{
    test_transpose<float>(rows, cols, mark);
    test_transpose<double>(rows, cols, mark);
    test_transpose<complex<float>>(rows, cols, mark);
    test_transpose<complex<double>>(rows, cols, mark);
}

TEST(matrix_transpose)
{
    constexpr size_t limit = 100;

    for (int i = 1; i <= limit; ++i)
    {
        for (int j = 1; j <= limit; ++j)
        {
            test_transpose_t(i, j);
        }
    }

    univector<int, 24> x = counter();
    matrix_transpose(x.data(), x.data(), shape{ 2, 3, 4 });
    CHECK(x == univector<int, 24>{ 0, 12, 4, 16, 8,  20, 1, 13, 5, 17, 9,  21,
                                   2, 14, 6, 18, 10, 22, 3, 15, 7, 19, 11, 23 });

    univector<uint8_t, 120> x2 = counter();
    matrix_transpose(x2.data(), x2.data(), shape{ 2, 3, 4, 5 });
    CHECK(x2 == univector<uint8_t, 120>{ 0,  60, 20,  80, 40, 100, 5,  65, 25,  85, 45, 105, 10, 70, 30,
                                         90, 50, 110, 15, 75, 35,  95, 55, 115, 1,  61, 21,  81, 41, 101,
                                         6,  66, 26,  86, 46, 106, 11, 71, 31,  91, 51, 111, 16, 76, 36,
                                         96, 56, 116, 2,  62, 22,  82, 42, 102, 7,  67, 27,  87, 47, 107,
                                         12, 72, 32,  92, 52, 112, 17, 77, 37,  97, 57, 117, 3,  63, 23,
                                         83, 43, 103, 8,  68, 28,  88, 48, 108, 13, 73, 33,  93, 53, 113,
                                         18, 78, 38,  98, 58, 118, 4,  64, 24,  84, 44, 104, 9,  69, 29,
                                         89, 49, 109, 14, 74, 34,  94, 54, 114, 19, 79, 39,  99, 59, 119 });

    tensor<int, 1> d{ shape{ 24 } };
    d                  = counter();
    tensor<int, 3> dd  = d.reshape(shape{ 2, 3, 4 });
    tensor<int, 3> ddd = dd.transpose();
    CHECK(trender(ddd.flatten_may_copy()) == tensor<int, 1>{ 0, 12, 4, 16, 8,  20, 1, 13, 5, 17, 9,  21,
                                                             2, 14, 6, 18, 10, 22, 3, 15, 7, 19, 11, 23 });
}

} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_MSVC(warning(pop))
