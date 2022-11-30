/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016-2022 Fractalium Ltd
 * See LICENSE.txt for details
 */

#include <kfr/base/random.hpp>
#include <kfr/base/reduce.hpp>
#include <kfr/io/tostring.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T, size_t N>
static void test_random(random_state& state, const vec<T, N>& value)
{
    const vec<T, N> r = kfr::random_uniform<T, N>(state);
    CHECK(r == value);
}

TEST(random_bit_generator)
{
    random_state gen = random_init(1, 2, 3, 4);
    test_random(gen, pack<u8>(21, 62, 88, 30, 46, 234, 205, 29, 41, 190, 212, 81, 217, 135, 218, 227));
    test_random(gen, pack<u16>(48589, 33814, 55928, 14799, 26904, 18521, 20808, 50888));
    test_random(gen, pack<u32>(1554764222, 1538765785, 2072590063, 2837641155));
    test_random(gen, pack<u64>(4036091275566340174, 2928916964115561767));

    test_random(gen, pack<i8>(-96, -23, -113, -39, 77, -9, 0, -32, 39, -52, 41, 78, 67, 89, 8, -78));
    test_random(gen, pack<i16>(31457, -7338, 25953, -9110, 19500, 20130, 918, -22379));
    test_random(gen, pack<i32>(546576509, -1598068238, 1068387634, 521513578));
    test_random(gen, pack<i64>(-1351534496328422696, -2056493566767233880));

    test_random(gen, pack<u8>(184));
    test_random(gen, pack<u16>(3766));
    test_random(gen, pack<u32>(2707944294));
    test_random(gen, pack<u64>(7319132094569945994));

    test_random(gen, pack<i8>(-87));
    test_random(gen, pack<i16>(30605));
    test_random(gen, pack<i32>(481763738));
    test_random(gen, pack<i64>(-4633152737592448342));

    test_random(
        gen, pack<f32>(0.68433606624603271, 0.33949351310729980, 0.99597716331481934, 0.71035039424896240));
    test_random(gen, pack<f64>(0.84738329802487988, 0.30307218960751059));
    test_random(gen, pack<f32>(0.35931551456451416));
    test_random(gen, pack<f64>(0.95290433236856908));

    test_random(gen, pack<u8>(218, 34, 127));
    test_random(gen, pack<u64>(9862453404643991575ull, 6719261899771853693, 7583499760963945490,
                               504102580557590315, 3864622132344054582));

    test_random(gen,
                pack<f32>(0.48961830139160156, 0.29450380802154541, 0.75503039360046387, 0.63871228694915771,
                          0.76648020744323730, 0.54290330410003662, 0.77374207973480225, 0.91389560699462891,
                          0.55802989006042480, 0.81261849403381348));
    test_random(gen,
                pack<f64>(0.87351817405232857, 0.07188206926267671, 0.45094433025385028, 0.11828513023601239,
                          0.48852715595764762, 0.73555664715112745, 0.60336462206956543, 0.70802907880871735,
                          0.66104424809495010, 0.65705152810593415, 0.94064561507444644, 0.33550309924374822,
                          0.80028288039450723));
}

TEST(gen_random_range)
{
    random_state gen         = random_init(1, 2, 3, 4);
    univector<fbase, 1000> v = gen_random_range<fbase>(std::ref(gen), -1.0, 1.0);
    CHECK(minof(v) >= fbase(-1.0));
    CHECK(maxof(v) <= fbase(1.0));
    // println(mean(v));
}

template <size_t Bins, typename E, typename TCount = uint32_t>
struct expression_histogram : public expression_with_traits<E>
{
    size_t size;
    using vector_type = univector<TCount, Bins == 0 ? tag_dynamic_vector : Bins>;
    mutable vector_type values{};

    using expression_with_traits<E>::expression_with_traits;
    
    KFR_MEM_INTRINSIC expression_histogram(E&& e, size_t steps) : expression_with_traits<E>{ std::forward<E>(e) }
    {
        if constexpr (Bins == 0)
        {
            values = vector_type(steps, 0);
        }
    }

    KFR_MEM_INTRINSIC TCount operator[](size_t n) const
    {
        KFR_LOGIC_CHECK(n < values.size() - 2, "n is outside histogram size");
        return values[1 + n];
    }
    KFR_MEM_INTRINSIC TCount below() const { return values.front(); }
    KFR_MEM_INTRINSIC TCount above() const { return values.back(); }
    KFR_MEM_INTRINSIC univector_ref<const TCount> histogram() const 
    {
        return values.slice(1, values.size());
    }

    using value_type = typename expression_with_traits<E>::value_type;

    template <index_t Axis, size_t N>
    friend KFR_INTRINSIC vec<value_type, N> get_elements(const expression_histogram& self,
                                                         const shape<expression_with_traits<E>::dims>& index,
                                                         const axis_params<Axis, N>& sh)
    {
        vec<value_type, N> v = get_elements(self.first(), index, sh);
        for (size_t i = 0; i < N; ++i)
        {
            int64_t n = 1 + std::floor(v[i] * (self.values.size() - 2));
            ++self.values[clamp(n, 0, self.values.size() - 1)];
        }
        return v;
    }
};

template <typename E, typename TCount = uint32_t>
KFR_INTRINSIC expression_histogram<0, E, TCount> histogram(E&& expr, size_t bins)
{
    return { std::forward<E>(expr), bins };
}

template <size_t Bins, typename E, typename TCount = uint32_t>
KFR_INTRINSIC expression_histogram<Bins, E, TCount> histogram(E&& expr)
{
    return { std::forward<E>(expr), Bins };
}

TEST(random_normal)
{
    random_state gen = random_init(1, 2, 3, 4);
    vec<fbase, 12> r = random_normal<12, fbase>(gen, 0.0, 1.0);
    println(r);
    r                 = random_normal<12, fbase>(gen, 0.0, 1.0);
    vec<fbase, 11> r2 = random_normal<11, fbase>(gen, 0.0, 1.0);
    println(r2);

    expression_histogram h = histogram<20>(gen_random_normal<double>() * 0.15 + 0.5);
    render(truncate(h, 1000));
    println(h.below());
    println(h.histogram());
    println(h.above());
    render(truncate(h, 10000));
    println(h.below());
    println(h.histogram());
    println(h.above());
    render(truncate(h, 100000));
    println(h.below());
    println(h.histogram());
    println(h.above());
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
