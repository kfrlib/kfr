/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/reduce.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/dsp/biquad.hpp>
#include <kfr/dsp/biquad_design.hpp>
#include <kfr/dsp/special.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T, typename... Ts, univector_tag Tag>
inline const univector<T, Tag>& choose_array(const univector<T, Tag>& array, const univector<Ts, Tag>&...)
{
    return array;
}

template <typename T, typename T2, typename... Ts, univector_tag Tag, KFR_ENABLE_IF(!std::is_same_v<T, T2>)>
inline const univector<T, Tag>& choose_array(const univector<T2, Tag>&, const univector<Ts, Tag>&... arrays)
{
    return choose_array<T>(arrays...);
}

TEST(biquad_lowpass1)
{
    testo::matrix(named("type") = ctypes_t<float, double>{},
                  [](auto type)
                  {
                      using T = typename decltype(type)::type;

                      const biquad_params<T> bq = biquad_lowpass<T>(0.1, 0.7);

                      constexpr size_t size = 32;

                      const univector<float, size> test_vector_f32{
                          +0x8.9bce2p-7,  +0xd.8383ep-6,  +0x8.f908dp-5,  +0xe.edc21p-6,  +0x9.ae104p-6,
                          +0x9.dcc24p-7,  +0xd.50584p-9,  -0xf.2668p-13,  -0xd.09ca1p-10, -0xe.15995p-10,
                          -0xa.b90d2p-10, -0xc.edea4p-11, -0xb.f14eap-12, -0xc.2cb44p-14, +0xb.4a4dep-15,
                          +0xb.685dap-14, +0xa.b181fp-14, +0xf.0cb2bp-15, +0x8.695d6p-15, +0xd.bedd4p-17,
                          +0xf.5474p-20,  -0xd.bb266p-19, -0x9.63ca1p-18, -0xf.ca567p-19, -0xa.5231p-19,
                          -0xa.9e934p-20, -0xe.ab52p-22,  +0xa.3c4cp-26,  +0xd.721ffp-23, +0xe.ccc1ap-23,
                          +0xb.5f248p-23, +0xd.d2c9ap-24,
                      };

                      const univector<double, size> test_vector_f64{
                          +0x8.9bce2bf3663e8p-7,  +0xd.8384010fdf1dp-6,   +0x8.f908e7a36df6p-5,
                          +0xe.edc2332a6d0bp-6,   +0x9.ae104af1da9ap-6,   +0x9.dcc235ef68e7p-7,
                          +0xd.5057ee425e05p-9,   -0xf.266e42a99aep-13,   -0xd.09cad73642208p-10,
                          -0xe.1599f32a83dp-10,   -0xa.b90d8910a117p-10,  -0xc.edeaabb890948p-11,
                          -0xb.f14edbb55383p-12,  -0xc.2cb39b86f2dap-14,  +0xb.4a506ecff055p-15,
                          +0xb.685edfdb55358p-14, +0xa.b182e32f8e298p-14, +0xf.0cb3dfd894b2p-15,
                          +0x8.695df725b4438p-15, +0xd.beddc3606b9p-17,   +0xf.547004d20874p-20,
                          -0xd.bb29b25b49b6p-19,  -0x9.63cb9187da1dp-18,  -0xf.ca588634fc618p-19,
                          -0xa.52322d320da78p-19, -0xa.9e9420154e4p-20,   -0xe.ab51f7b0335ap-22,
                          +0xa.3c6479980e1p-26,   +0xd.7223836599fp-23,   +0xe.ccc47ddd18678p-23,
                          +0xb.5f265b1be1728p-23, +0xd.d2cb83f8483f8p-24,
                      };

                      const univector<T, size> ir = biquad(bq, unitimpulse<T>());

                      CHECK(absmaxof(choose_array<T>(test_vector_f32, test_vector_f64) - ir) == 0);
                  });
}

TEST(biquad_lowpass2)
{
    testo::matrix(named("type") = ctypes_t<float, double>{},
                  [](auto type)
                  {
                      using T = typename decltype(type)::type;

                      const biquad_params<T> bq = biquad_lowpass<T>(0.45, 0.2);

                      constexpr size_t size = 32;

                      const univector<float, size> test_vector_f32{
                          +0x8.ce416p-4,  +0x8.2979p-4,   -0x8.a9d04p-7,  +0xe.aeb3p-11,  +0x8.204f8p-13,
                          -0x8.20d78p-12, +0x8.3379p-12,  -0xf.83d81p-13, +0xe.8b5c4p-13, -0xd.9ddadp-13,
                          +0xc.bedfcp-13, -0xb.ee123p-13, +0xb.2a9e5p-13, -0xa.73ac4p-13, +0x9.c86f6p-13,
                          -0x9.2828p-13,  +0x8.92229p-13, -0x8.05b7p-13,  +0xf.048ffp-14, -0xe.0e849p-14,
                          +0xd.28384p-14, -0xc.50a9p-14,  +0xb.86e56p-14, -0xa.ca0b6p-14, +0xa.19476p-14,
                          -0x9.73d38p-14, +0x8.d8f64p-14, -0x8.48024p-14, +0xf.80aa2p-15, -0xe.82ad8p-15,
                          +0xd.94f22p-15, -0xc.b66d9p-15,
                      };

                      const univector<double, size> test_vector_f64{
                          +0x8.ce416c0d31e88p-4,  +0x8.2978efe51dafp-4,   -0x8.a9d088b81da6p-7,
                          +0xe.aeb56c029358p-11,  +0x8.20492639873ap-13,  -0x8.20d4e21aab538p-12,
                          +0x8.3376b2d53b4a8p-12, -0xf.83d3d1c17343p-13,  +0xe.8b584f0dd5ac8p-13,
                          -0xd.9dd740ceaacf8p-13, +0xc.bedc85e7a621p-13,  -0xb.ee0f472bf8968p-13,
                          +0xb.2a9baed1fe6cp-13,  -0xa.73a9d1670f4ep-13,  +0x9.c86d29d297798p-13,
                          -0x9.2825f4d894088p-13, +0x8.9220a956d651p-13,  -0x8.05b539fdd79e8p-13,
                          +0xf.048cb5194cfa8p-14, -0xe.0e819fa128938p-14, +0xd.2835957d684cp-14,
                          -0xc.50a69c2a8dc18p-14, +0xb.86e33bbaf3cbp-14,  -0xa.ca097058af2cp-14,
                          +0xa.1945ad1703dcp-14,  -0x9.73d1eef7d8b68p-14, +0x8.d8f4df1bb3efp-14,
                          -0x8.48010323c6f7p-14,  +0xf.80a7f5baeeb2p-15,  -0xe.82ab94bb68a8p-15,
                          +0xd.94f05f80af008p-15, -0xc.b66c0799b21a8p-15,
                      };

                      const univector<T, size> ir = biquad(bq, unitimpulse<T>());

                      CHECK(absmaxof(choose_array<T>(test_vector_f32, test_vector_f64) - ir) == 0);
                  });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
