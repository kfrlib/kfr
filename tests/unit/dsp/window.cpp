/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/reduce.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/tensor.hpp>
#include <kfr/dsp/window.hpp>
#include <kfr/io/tostring.hpp>

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4305))
CMT_PRAGMA_MSVC(warning(disable : 4244))

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

const char* wins[] = {
    "",
    "rectangular    ",
    "triangular     ",
    "bartlett       ",
    "cosine         ",
    "hann           ",
    "bartlett_hann  ",
    "hamming        ",
    "bohman         ",
    "blackman       ",
    "blackman_harris",
    "kaiser         ",
    "flattop        ",
    "gaussian       ",
    "lanczos        ",
    "cosine_np      ",
    "planck_taper   ",
    "tukey          ",
};

template <window_type type, typename T>
void win(size_t len, T arg, window_symmetry sym, univector<T> ref)
{
    univector<T> calc = render(window(len, cval_t<window_type, type>{}, arg, sym, cometa::ctype<T>));
    testo::scope sc(as_string("win=", wins[static_cast<int>(type)], " len=", len, " sym=",
                              sym == window_symmetry::symmetric, "\n    calc=", calc, "\n    ref =", ref));
    CHECK(rms(calc - ref) < 0.00001f);
}

TEST(window)
{
    using w = window_type;
    using s = window_symmetry;
    using u = univector<f32>;
    // clang-format
    // 7 ; symmetric
    win<w::rectangular, f32>(7, 0.0, s::symmetric, u{ 1, 1, 1, 1, 1, 1, 1 });
    win<w::triangular, f32>(7, 0.0, s::symmetric, u{ 0.25, 0.5, 0.75, 1., 0.75, 0.5, 0.25 });
    win<w::bartlett, f32>(7, 0.0, s::symmetric,
                          u{ 0., 0.33333333, 0.66666667, 1., 0.66666667, 0.33333333, 0. });
    win<w::cosine, f32>(7, 0.0, s::symmetric, u{ 0, 0.5, 0.866025, 1, 0.866025, 0.5, 0 });
    win<w::hann, f32>(7, 0.0, s::symmetric, u{ 0., 0.25, 0.75, 1., 0.75, 0.25, 0. });
    win<w::bartlett_hann, f32>(7, 0.0, s::symmetric, u{ 0., 0.27, 0.73, 1., 0.73, 0.27, 0. });
    win<w::hamming, f32>(7, 0.54, s::symmetric, u{ 0.08, 0.31, 0.77, 1., 0.77, 0.31, 0.08 });
    win<w::bohman, f32>(7, 0.0, s::symmetric,
                        u{ 0., 0.10899778, 0.60899778, 1., 0.60899778, 0.10899778, 0. });
    win<w::blackman, f32>(7, 0.16, s::symmetric,
                          u{ -1.38777878e-17, 1.30000000e-01, 6.30000000e-01, 1.00000000e+00, 6.30000000e-01,
                             1.30000000e-01, -1.38777878e-17 });
    win<w::blackman_harris, f32>(
        7, 0.0, s::symmetric,
        u{ 6.00000e-05, 5.56450e-02, 5.20575e-01, 1.00000e+00, 5.20575e-01, 5.56450e-02, 6.00000e-05 });
    win<w::kaiser, f32>(7, 8.0, s::symmetric,
                        u{ 0.00233883, 0.1520107, 0.65247867, 1., 0.65247867, 0.1520107, 0.00233883 });
    win<w::flattop, f32>(7, 0.0, s::symmetric,
                         u{ -4.2105100e-04, -5.1263156e-02, 1.9821053e-01, 1.0000000e+00, 1.9821053e-01,
                            -5.1263156e-02, -4.2105100e-04 });
    win<w::gaussian, f32>(7, 2.5, s::symmetric,
                          u{ 0.1006689, 0.36044779, 0.77483743, 1., 0.77483743, 0.36044779, 0.1006689 });
    win<w::lanczos, f32>(7, 0.0, s::symmetric,
                         u{ -2.8e-08, 0.413497, 0.826993, 1, 0.826993, 0.413497, -2.8e-08 });
    win<w::cosine_np, f32>(7, 0.0, s::symmetric,
                           u{ 0.22252093, 0.6234898, 0.90096887, 1., 0.90096887, 0.6234898, 0.22252093 });
    win<w::planck_taper, f32>(7, 0.25, s::symmetric, u{ 0, 0.817575, 1, 1, 1, 0.817574, 0 });
    win<w::tukey, f32>(7, 0.5, s::symmetric, u{ 0., 0.75, 1., 1., 1., 0.75, 0. });

    // 8 ; symmetric
    win<w::rectangular, f32>(8, 0.0, s::symmetric, u{ 1, 1, 1, 1, 1, 1, 1, 1 });
    win<w::triangular, f32>(8, 0.0, s::symmetric,
                            u{ 0.125, 0.375, 0.625, 0.875, 0.875, 0.625, 0.375, 0.125 });
    win<w::bartlett, f32>(
        8, 0.0, s::symmetric,
        u{ 0., 0.28571429, 0.57142857, 0.85714286, 0.85714286, 0.57142857, 0.28571429, 0. });
    win<w::cosine, f32>(8, 0.0, s::symmetric,
                        u{ 0, 0.433884, 0.781832, 0.974928, 0.974928, 0.781831, 0.433883, 0 });
    win<w::hann, f32>(8, 0.0, s::symmetric,
                      u{ 0., 0.1882551, 0.61126047, 0.95048443, 0.95048443, 0.61126047, 0.1882551, 0. });
    win<w::bartlett_hann, f32>(
        8, 0.0, s::symmetric,
        u{ 0., 0.2116453, 0.60170081, 0.92808246, 0.92808246, 0.60170081, 0.2116453, 0. });
    win<w::hamming, f32>(
        8, 0.54, s::symmetric,
        u{ 0.08, 0.25319469, 0.64235963, 0.95444568, 0.95444568, 0.64235963, 0.25319469, 0.08 });
    win<w::bohman, f32>(8, 0.0, s::symmetric,
                        u{ 0., 0.07072475, 0.43748401, 0.91036851, 0.91036851, 0.43748401, 0.07072475, 0. });
    win<w::blackman, f32>(8, 0.16, s::symmetric,
                          u{ -1.38777878e-17, 9.04534244e-02, 4.59182958e-01, 9.20363618e-01, 9.20363618e-01,
                             4.59182958e-01, 9.04534244e-02, -1.38777878e-17 });
    win<w::blackman_harris, f32>(8, 0.0, s::symmetric,
                                 u{ 6.00000000e-05, 3.33917235e-02, 3.32833504e-01, 8.89369772e-01,
                                    8.89369772e-01, 3.32833504e-01, 3.33917235e-02, 6.00000000e-05 });
    win<w::kaiser, f32>(
        8, 8.0, s::symmetric,
        u{ 0.00233883, 0.10919581, 0.48711868, 0.92615774, 0.92615774, 0.48711868, 0.10919581, 0.00233883 });
    win<w::flattop, f32>(8, 0.0, s::symmetric,
                         u{ -4.21051000e-04, -3.68407812e-02, 1.07037167e-02, 7.80873915e-01, 7.80873915e-01,
                            1.07037167e-02, -3.68407812e-02, -4.21051000e-04 });
    win<w::gaussian, f32>(
        8, 2.5, s::symmetric,
        u{ 0.09139376, 0.29502266, 0.64438872, 0.9523448, 0.9523448, 0.64438872, 0.29502266, 0.09139376 });
    win<w::lanczos, f32>(8, 0.0, s::symmetric,
                         u{ -2.8e-08, 0.348411, 0.724101, 0.966766, 0.966766, 0.724101, 0.34841, -2.8e-08 });
    win<w::cosine_np, f32>(
        8, 0.0, s::symmetric,
        u{ 0.19509032, 0.55557023, 0.83146961, 0.98078528, 0.98078528, 0.83146961, 0.55557023, 0.19509032 });
    win<w::planck_taper, f32>(8, 0.25, s::symmetric, u{ 0, 0.641834, 1, 1, 1, 1, 0.641833, 0 });
    win<w::tukey, f32>(8, 0.5, s::symmetric, u{ 0., 0.61126047, 1., 1., 1., 1., 0.61126047, 0. });

    // 7 ; periodic
    win<w::rectangular, f32>(7, 0.0, s::periodic, u{ 1, 1, 1, 1, 1, 1, 1 });
    win<w::triangular, f32>(7, 0.0, s::periodic, u{ 0.125, 0.375, 0.625, 0.875, 0.875, 0.625, 0.375 });
    win<w::bartlett, f32>(7, 0.0, s::periodic,
                          u{ 0., 0.28571429, 0.57142857, 0.85714286, 0.85714286, 0.57142857, 0.28571429 });
    win<w::cosine, f32>(7, 0.0, s::periodic,
                        u{ 0, 0.433884, 0.781832, 0.974928, 0.974928, 0.781831, 0.433883 });
    win<w::hann, f32>(7, 0.0, s::periodic,
                      u{ 0., 0.1882551, 0.61126047, 0.95048443, 0.95048443, 0.61126047, 0.1882551 });
    win<w::bartlett_hann, f32>(7, 0.0, s::periodic,
                               u{ 0., 0.2116453, 0.60170081, 0.92808246, 0.92808246, 0.60170081, 0.2116453 });
    win<w::hamming, f32>(7, 0.54, s::periodic,
                         u{ 0.08, 0.25319469, 0.64235963, 0.95444568, 0.95444568, 0.64235963, 0.25319469 });
    win<w::bohman, f32>(7, 0.0, s::periodic,
                        u{ 0., 0.07072475, 0.43748401, 0.91036851, 0.91036851, 0.43748401, 0.07072475 });
    win<w::blackman, f32>(7, 0.16, s::periodic,
                          u{ -1.38777878e-17, 9.04534244e-02, 4.59182958e-01, 9.20363618e-01, 9.20363618e-01,
                             4.59182958e-01, 9.04534244e-02 });
    win<w::blackman_harris, f32>(7, 0.0, s::periodic,
                                 u{ 6.00000000e-05, 3.33917235e-02, 3.32833504e-01, 8.89369772e-01,
                                    8.89369772e-01, 3.32833504e-01, 3.33917235e-02 });
    win<w::kaiser, f32>(
        7, 8.0, s::periodic,
        u{ 0.00233883, 0.10919581, 0.48711868, 0.92615774, 0.92615774, 0.48711868, 0.10919581 });
    win<w::flattop, f32>(7, 0.0, s::periodic,
                         u{ -4.21051000e-04, -3.68407812e-02, 1.07037167e-02, 7.80873915e-01, 7.80873915e-01,
                            1.07037167e-02, -3.68407812e-02 });
    win<w::gaussian, f32>(
        7, 2.5, s::periodic,
        u{ 0.09139376, 0.29502266, 0.64438872, 0.9523448, 0.9523448, 0.64438872, 0.29502266 });
    win<w::lanczos, f32>(7, 0.0, s::periodic,
                         u{ -2.8e-08, 0.348411, 0.724101, 0.966766, 0.966766, 0.724101, 0.34841 });
    win<w::cosine_np, f32>(
        7, 0.0, s::periodic,
        u{ 0.19509032, 0.55557023, 0.83146961, 0.98078528, 0.98078528, 0.83146961, 0.55557023 });
    win<w::planck_taper, f32>(7, 0.25, s::periodic, u{ 0, 0.641834, 1, 1, 1, 1, 0.641833 });
    win<w::tukey, f32>(7, 0.5, s::periodic, u{ 0., 0.61126047, 1., 1., 1., 1., 0.61126047 });

    // 8 ; periodic
    win<w::rectangular, f32>(8, 0.0, s::periodic, u{ 1, 1, 1, 1, 1, 1, 1, 1 });
    win<w::triangular, f32>(8, 0.0, s::periodic, u{ 0.2, 0.4, 0.6, 0.8, 1., 0.8, 0.6, 0.4 });
    win<w::bartlett, f32>(8, 0.0, s::periodic, u{ 0., 0.25, 0.5, 0.75, 1., 0.75, 0.5, 0.25 });
    win<w::cosine, f32>(8, 0.0, s::periodic,
                        u{ 0, 0.382683, 0.707107, 0.92388, 1, 0.92388, 0.707107, 0.382683 });
    win<w::hann, f32>(8, 0.0, s::periodic,
                      u{ 0., 0.14644661, 0.5, 0.85355339, 1., 0.85355339, 0.5, 0.14644661 });
    win<w::bartlett_hann, f32>(8, 0.0, s::periodic,
                               u{ 0., 0.17129942, 0.5, 0.82870058, 1., 0.82870058, 0.5, 0.17129942 });
    win<w::hamming, f32>(8, 0.54, s::periodic,
                         u{ 0.08, 0.21473088, 0.54, 0.86526912, 1., 0.86526912, 0.54, 0.21473088 });
    win<w::bohman, f32>(8, 0.0, s::periodic,
                        u{ 0., 0.04830238, 0.31830989, 0.75540916, 1., 0.75540916, 0.31830989, 0.04830238 });
    win<w::blackman, f32>(8, 0.16, s::periodic,
                          u{ -1.38777878e-17, 6.64466094e-02, 3.40000000e-01, 7.73553391e-01, 1.00000000e+00,
                             7.73553391e-01, 3.40000000e-01, 6.64466094e-02 });
    win<w::blackman_harris, f32>(8, 0.0, s::periodic,
                                 u{ 6.00000000e-05, 2.17358370e-02, 2.17470000e-01, 6.95764163e-01,
                                    1.00000000e+00, 6.95764163e-01, 2.17470000e-01, 2.17358370e-02 });
    win<w::kaiser, f32>(
        8, 8.0, s::periodic,
        u{ 0.00233883, 0.08273982, 0.36897272, 0.78875245, 1., 0.78875245, 0.36897272, 0.08273982 });
    win<w::flattop, f32>(8, 0.0, s::periodic,
                         u{ -4.21051000e-04, -2.68721933e-02, -5.47368400e-02, 4.44135357e-01, 1.00000000e+00,
                            4.44135357e-01, -5.47368400e-02, -2.68721933e-02 });
    win<w::gaussian, f32>(
        8, 2.5, s::periodic,
        u{ 0.08465799, 0.24935221, 0.53940751, 0.85699689, 1., 0.85699689, 0.53940751, 0.24935221 });
    win<w::lanczos, f32>(8, 0.0, s::periodic,
                         u{ -2.8e-08, 0.300105, 0.63662, 0.900316, 1, 0.900316, 0.63662, 0.300105 });
    win<w::cosine_np, f32>(8, 0.0, s::periodic,
                           u{ 0.17364818, 0.5, 0.76604444, 0.93969262, 1., 0.93969262, 0.76604444, 0.5 });
    win<w::planck_taper, f32>(8, 0.25, s::periodic, u{ 0, 0.5, 1, 1, 1, 1, 1, 0.5 });
    win<w::tukey, f32>(8, 0.5, s::periodic, u{ 0., 0.5, 1., 1., 1., 1., 1., 0.5 });
    // clang-format on
}
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_MSVC(warning(pop))
