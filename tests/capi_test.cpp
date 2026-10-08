#define KFR_NO_C_COMPLEX_TYPES 1

#include <kfr/capi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

#ifdef _MSC_VER
#define CHECK(condition, message, ...)                                                                       \
    if (!(condition))                                                                                        \
    {                                                                                                        \
        ++failures;                                                                                          \
        fprintf(stderr, "[FAILED] " message "\n", __VA_ARGS__);                                              \
    }
#else
#define CHECK(condition, message, ...)                                                                       \
    if (!(condition))                                                                                        \
    {                                                                                                        \
        ++failures;                                                                                          \
        fprintf(stderr, "[FAILED] " message "\n", ##__VA_ARGS__);                                            \
    }
#endif

void test_memory()
{
    printf("[TEST] Memory allocation\n");
    uint8_t* d = (uint8_t*)(kfr_allocate(256));
    for (size_t i = 0; i < 256; i++)
        d[i] = i;
#ifdef KFR_MANAGED_ALLOCATION
    CHECK(kfr_allocated_size(d) == 256, "kfr_allocated_size: wrong size: %zu", kfr_allocated_size(d));
    d = (uint8_t*)(kfr_reallocate(d, 512));
    CHECK(kfr_allocated_size(d) == 512, "kfr_allocated_size: wrong size: %zu", kfr_allocated_size(d));
    for (size_t i = 0; i < 256; i++)
    {
        CHECK(d[i] == i, "kfr_reallocate: data lost after reallocation\n");
    }
#endif
    kfr_deallocate(d);

    void* page = kfr_allocate_aligned(4096, 4096);
    CHECK(((uintptr_t)page & 0xFFF) == 0, "kfr_allocate_aligned: wrong alignment: 0x%zx",
          ((uintptr_t)page & 0xFFF));

    kfr_deallocate(page);
}

#define DFT_SIZE 32

static void init_data_f32(kfr_f32* buf)
{
    for (int i = 0; i < DFT_SIZE; i++)
    {
        buf[i * 2 + 0] = (float)(i) / DFT_SIZE;
        buf[i * 2 + 1] = (float)(-i) / DFT_SIZE;
    }
}
static void init_data_f64(kfr_f64* buf)
{
    for (int i = 0; i < DFT_SIZE; i++)
    {
        buf[i * 2 + 0] = (double)(i) / DFT_SIZE;
        buf[i * 2 + 1] = (double)(-i) / DFT_SIZE;
    }
}

static void test_data_f32(kfr_f32* buf)
{
    const float eps = 0.00001f;
    for (int i = 0; i < DFT_SIZE; i++)
    {
        CHECK(fabsf(buf[i * 2 + 0] - (float)(i)) < eps, "DFT: wrong result at %d: re = %f", i,
              buf[i * 2 + 0]);
        CHECK(fabsf(buf[i * 2 + 1] - (float)(-i)) < eps, "DFT: wrong result at %d: im = %f", i,
              buf[i * 2 + 1]);
    }
}
static void test_data_f64(kfr_f64* buf)
{
    const double eps = 0.00001;
    for (int i = 0; i < DFT_SIZE; i++)
    {
        CHECK(fabs(buf[i * 2 + 0] - (double)(i)) < eps, "DFT: wrong result at %d: re = %f", i,
              buf[i * 2 + 0]);
        CHECK(fabs(buf[i * 2 + 1] - (double)(-i)) < eps, "DFT: wrong result at %d: im = %f", i,
              buf[i * 2 + 1]);
    }
}

void test_dft_f32()
{
    printf("[TEST] DFT f32\n");
    // kfr_dft_dump_f32(plan);
    kfr_f32 buf[DFT_SIZE * 2];
    KFR_DFT_PLAN_F32* plan;
    uint8_t* tmp;

    init_data_f32(buf);
    plan = kfr_dft_create_plan_f32(DFT_SIZE);
    tmp  = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f32(plan));
    kfr_dft_execute_f32(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f32(plan, buf, buf, tmp);
    kfr_deallocate(tmp);
    kfr_dft_delete_plan_f32(plan);
    test_data_f32(buf);

    init_data_f32(buf);
    plan = kfr_dft_create_2d_plan_f32(8, 4);
    tmp  = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f32(plan));
    kfr_dft_execute_f32(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f32(plan, buf, buf, tmp);
    kfr_deallocate(tmp);
    kfr_dft_delete_plan_f32(plan);
    test_data_f32(buf);

    init_data_f32(buf);
    plan = kfr_dft_create_3d_plan_f32(4, 4, 2);
    tmp  = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f32(plan));
    kfr_dft_execute_f32(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f32(plan, buf, buf, tmp);
    kfr_deallocate(tmp);
    kfr_dft_delete_plan_f32(plan);
    test_data_f32(buf);

    unsigned sizes[4] = { 2, 2, 2, 4 };
    init_data_f32(buf);
    plan = kfr_dft_create_md_plan_f32(4, sizes);
    tmp  = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f32(plan));
    kfr_dft_execute_f32(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f32(plan, buf, buf, tmp);
    kfr_deallocate(tmp);
    kfr_dft_delete_plan_f32(plan);
    test_data_f32(buf);
}

void test_dft_f64()
{
    printf("[TEST] DFT f64\n");
    // kfr_dft_dump_f64(plan);
    kfr_f64 buf[DFT_SIZE * 2];
    KFR_DFT_PLAN_F64* plan;
    uint8_t* tmp;

    init_data_f64(buf);
    plan = kfr_dft_create_plan_f64(DFT_SIZE);
    tmp  = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f64(plan));
    kfr_dft_execute_f64(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f64(plan, buf, buf, tmp);
    kfr_deallocate(tmp);
    kfr_dft_delete_plan_f64(plan);
    test_data_f64(buf);

    init_data_f64(buf);
    plan = kfr_dft_create_2d_plan_f64(8, 4);
    tmp  = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f64(plan));
    kfr_dft_execute_f64(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f64(plan, buf, buf, tmp);
    kfr_deallocate(tmp);
    kfr_dft_delete_plan_f64(plan);
    test_data_f64(buf);

    init_data_f64(buf);
    plan = kfr_dft_create_3d_plan_f64(4, 4, 2);
    tmp  = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f64(plan));
    kfr_dft_execute_f64(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f64(plan, buf, buf, tmp);
    kfr_deallocate(tmp);
    kfr_dft_delete_plan_f64(plan);
    test_data_f64(buf);

    unsigned sizes[4] = { 2, 2, 2, 4 };
    init_data_f64(buf);
    plan = kfr_dft_create_md_plan_f64(4, sizes);
    tmp  = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f64(plan));
    kfr_dft_execute_f64(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f64(plan, buf, buf, tmp);
    kfr_deallocate(tmp);
    kfr_dft_delete_plan_f64(plan);
    test_data_f64(buf);
}

#define FILTER_SIZE 256

void test_fir_f32()
{
    printf("[TEST] FIR f32\n");
    kfr_f32 taps[]         = { 1.f, 2.f, -2.f, -1.f };
    KFR_FILTER_F32* filter = kfr_filter_create_fir_plan_f32(taps, sizeof(taps) / sizeof(kfr_f32));

    kfr_f32 buf[FILTER_SIZE];
    for (int i = 0; i < FILTER_SIZE; i++)
        buf[i] = i;

    kfr_filter_process_f32(filter, buf, buf, FILTER_SIZE);
    CHECK(buf[0] == 0, "FIR: wrong result at %d: %g", 0, buf[0]);
    CHECK(buf[1] == 1, "FIR: wrong result at %d: %g", 1, buf[1]);
    CHECK(buf[2] == 4, "FIR: wrong result at %d: %g", 2, buf[2]);
    CHECK(buf[3] == 5, "FIR: wrong result at %d: %g", 3, buf[3]);
    CHECK(buf[FILTER_SIZE - 1] == 5, "FIR: wrong result at %d: %g", FILTER_SIZE - 1, buf[FILTER_SIZE - 1]);

    kfr_filter_delete_plan_f32(filter);
}

void test_fir_f64()
{
    printf("[TEST] FIR f64\n");
    kfr_f64 taps[]         = { 1.f, 2.f, -2.f, -1.f };
    KFR_FILTER_F64* filter = kfr_filter_create_fir_plan_f64(taps, sizeof(taps) / sizeof(kfr_f64));

    kfr_f64 buf[FILTER_SIZE];
    for (int i = 0; i < FILTER_SIZE; i++)
        buf[i] = i;

    kfr_filter_process_f64(filter, buf, buf, FILTER_SIZE);
    CHECK(buf[0] == 0, "FIR: wrong result at %d: %g", 0, buf[0]);
    CHECK(buf[1] == 1, "FIR: wrong result at %d: %g", 1, buf[1]);
    CHECK(buf[2] == 4, "FIR: wrong result at %d: %g", 2, buf[2]);
    CHECK(buf[3] == 5, "FIR: wrong result at %d: %g", 3, buf[3]);
    CHECK(buf[FILTER_SIZE - 1] == 5, "FIR: wrong result at %d: %g", FILTER_SIZE - 1, buf[FILTER_SIZE - 1]);

    kfr_filter_delete_plan_f64(filter);
}

void test_iir_f32()
{
    const float eps = 0.00001f;
    printf("[TEST] IIR f32\n");
    float sos[6] = {
        1.,
        -1.872871474946867,
        0.8809814578599688,
        0.002027495728275458,
        0.004054991456550916,
        0.002027495728275458,
    };
    KFR_FILTER_F32* filter = kfr_filter_create_iir_plan_f32(sos, 1);

    kfr_f32 buf[FILTER_SIZE];
    kfr_f32 src[4] = { 0, 1, 0, -1 };
    for (int i = 0; i < FILTER_SIZE; i++)
        buf[i] = src[i % 4];

    kfr_filter_process_f32(filter, buf, buf, FILTER_SIZE);

    CHECK(fabsf(buf[0] - 0.f) < eps, "IIR: wrong result at %d: %f", 0, buf[0]);
    CHECK(fabsf(buf[1] - 0.002027496f) < eps, "IIR: wrong result at %d: %f", 1, buf[1]);
    CHECK(fabsf(buf[60] - -0.001285130f) < eps, "IIR: wrong result at %d: %f", 60, buf[60]);

    kfr_filter_delete_plan_f32(filter);
}

void test_iir_f64()
{
    const double eps = 0.0000001;
    printf("[TEST] IIR f64\n");
    double sos[6] = {
        1.,
        -1.872871474946867,
        0.8809814578599688,
        0.002027495728275458,
        0.004054991456550916,
        0.002027495728275458,
    };
    KFR_FILTER_F64* filter = kfr_filter_create_iir_plan_f64(sos, 1);

    kfr_f64 buf[FILTER_SIZE];
    kfr_f64 src[4] = { 0, 1, 0, -1 };
    for (int i = 0; i < FILTER_SIZE; i++)
        buf[i] = src[i % 4];

    kfr_filter_process_f64(filter, buf, buf, FILTER_SIZE);

    CHECK(fabs(buf[0] - 0.) < eps, "IIR: wrong result at %d: %f", 0, buf[0]);
    CHECK(fabs(buf[1] - 0.002027496) < eps, "IIR: wrong result at %d: %f", 1, buf[1]);
    CHECK(fabs(buf[60] - -0.001285130) < eps, "IIR: wrong result at %d: %f", 60, buf[60]);

    kfr_filter_delete_plan_f64(filter);
}

#define REF_TOL_F32 1e-5
#define REF_TOL_F64 1e-9

// Reference values from scipy.signal at fs = 48000 Hz. IIR sections use KFR order (a0, a1, a2, b0, b1, b2).
static const double ref_butter2_lp_sos[6] = { 1.0,
                                              -1.815341082704568,
                                              0.8310055893467575,
                                              0.0039161266605473692,
                                              0.0078322533210947384,
                                              0.0039161266605473692 };
static const double ref_butter2_lp_gain_1k    = 0.70710678118654702;
static const double ref_cheby1_4_lp_gain[3]   = { 0.96476846740354505, 0.89125093813374912,
                                                0.0027846751465542539 };
static const double ref_fir_lp11[11]          = { 0.01374731529858713,  0.029613366989267531, 0.071629337393071646,
                                                  0.12459353610695942,  0.16804038774446348,  0.18475211293530147,
                                                  0.16804038774446348,  0.12459353610695942,  0.071629337393071646,
                                                  0.029613366989267531, 0.01374731529858713 };
static const double ref_hann_sym9[9]          = { 0.0, 0.14644660940672627, 0.5, 0.85355339059327373, 1.0,
                                                  0.85355339059327373, 0.5, 0.14644660940672627, 0.0 };
static const double ref_hamming_per8[8]       = { 0.080000000000000071, 0.21473088065418822, 0.54000000000000004,
                                                  0.86526911934581197, 1.0, 0.86526911934581197,
                                                  0.54000000000000004, 0.21473088065418822 };

static void check_near(const char* what, size_t index, double got, double expected, double tol)
{
    CHECK(fabs(got - expected) <= tol, "%s[%zu]: got %.12g, expected %.12g", what, index, got, expected);
}

static void to_double(const kfr_f32* src, double* dst, size_t count)
{
    for (size_t i = 0; i < count; i++)
        dst[i] = src[i];
}

static double poly_magnitude(double c0, double c1, double c2, double w)
{
    const double re = c0 + c1 * cos(w) + c2 * cos(2.0 * w);
    const double im = c1 * sin(w) + c2 * sin(2.0 * w);
    return sqrt(re * re + im * im);
}

static double sos_magnitude(const double* sos, size_t sections, double hz, double fs)
{
    const double w = 2.0 * 3.14159265358979323846 * hz / fs;
    double mag     = 1.0;
    for (size_t s = 0; s < sections; s++)
    {
        const double* c = sos + 6 * s;
        mag *= poly_magnitude(c[3], c[4], c[5], w) / poly_magnitude(c[0], c[1], c[2], w);
    }
    return mag;
}

static void check_butter2_sos(const char* what, const double* sos, double tol)
{
    for (size_t k = 0; k < 6; k++)
        check_near(what, k, sos[k] / sos[0], ref_butter2_lp_sos[k], tol);
}

void test_reference_iir_f32()
{
    printf("[TEST] IIR design vs scipy f32\n");
    kfr_f32 sos32[6 * 4];
    double sos[6 * 4];

    size_t n = kfr_iir_design_f32(KFR_IIR_BUTTERWORTH, KFR_IIR_LOWPASS, 2, 1.f, 1.f, 1000.f, 0.f, 48000.f,
                                  sos32, 4);
    CHECK(n == 1, "IIR butterworth: expected 1 section, got %zu (%s)", n, kfr_last_error());
    if (n == 1)
    {
        to_double(sos32, sos, 6 * n);
        check_butter2_sos("IIR butterworth f32 sos", sos, REF_TOL_F32);
        check_near("IIR butterworth f32 gain", 0, sos_magnitude(sos, n, 1000.0, 48000.0),
                   ref_butter2_lp_gain_1k, REF_TOL_F32);
    }

    n = kfr_iir_design_f32(KFR_IIR_CHEBYSHEV1, KFR_IIR_LOWPASS, 4, 1.f, 1.f, 2000.f, 0.f, 48000.f, sos32, 4);
    CHECK(n == 2, "IIR chebyshev1: expected 2 sections, got %zu (%s)", n, kfr_last_error());
    if (n == 2)
    {
        to_double(sos32, sos, 6 * n);
        const double freqs[3] = { 500.0, 2000.0, 6000.0 };
        for (size_t i = 0; i < 3; i++)
            check_near("IIR chebyshev1 f32 gain", i, sos_magnitude(sos, n, freqs[i], 48000.0),
                       ref_cheby1_4_lp_gain[i], REF_TOL_F32);
    }
}

void test_reference_iir_f64()
{
    printf("[TEST] IIR design vs scipy f64\n");
    kfr_f64 sos[6 * 4];

    size_t n = kfr_iir_design_f64(KFR_IIR_BUTTERWORTH, KFR_IIR_LOWPASS, 2, 1.0, 1.0, 1000.0, 0.0, 48000.0, sos, 4);
    CHECK(n == 1, "IIR butterworth: expected 1 section, got %zu (%s)", n, kfr_last_error());
    if (n == 1)
    {
        check_butter2_sos("IIR butterworth f64 sos", sos, REF_TOL_F64);
        check_near("IIR butterworth f64 gain", 0, sos_magnitude(sos, n, 1000.0, 48000.0),
                   ref_butter2_lp_gain_1k, REF_TOL_F64);
    }

    n = kfr_iir_design_f64(KFR_IIR_CHEBYSHEV1, KFR_IIR_LOWPASS, 4, 1.0, 1.0, 2000.0, 0.0, 48000.0, sos, 4);
    CHECK(n == 2, "IIR chebyshev1: expected 2 sections, got %zu (%s)", n, kfr_last_error());
    if (n == 2)
    {
        const double freqs[3] = { 500.0, 2000.0, 6000.0 };
        for (size_t i = 0; i < 3; i++)
            check_near("IIR chebyshev1 f64 gain", i, sos_magnitude(sos, n, freqs[i], 48000.0),
                       ref_cheby1_4_lp_gain[i], REF_TOL_F64);
    }
}

void test_reference_fir_f32()
{
    printf("[TEST] FIR design vs scipy f32\n");
    kfr_f32 taps[11];
    kfr_bool ok = kfr_fir_design_f32(KFR_FIR_LOWPASS, KFR_WINDOW_HAMMING, 0.54f, 1000.f, 0.f, 48000.f, 1, taps, 11);
    CHECK(ok, "FIR lowpass f32: design failed (%s)", kfr_last_error());
    if (ok)
        for (size_t i = 0; i < 11; i++)
            check_near("FIR lowpass f32 taps", i, taps[i], ref_fir_lp11[i], REF_TOL_F32);
}

void test_reference_fir_f64()
{
    printf("[TEST] FIR design vs scipy f64\n");
    kfr_f64 taps[11];
    kfr_bool ok = kfr_fir_design_f64(KFR_FIR_LOWPASS, KFR_WINDOW_HAMMING, 0.54, 1000.0, 0.0, 48000.0, 1, taps, 11);
    CHECK(ok, "FIR lowpass f64: design failed (%s)", kfr_last_error());
    if (ok)
        for (size_t i = 0; i < 11; i++)
            check_near("FIR lowpass f64 taps", i, taps[i], ref_fir_lp11[i], REF_TOL_F64);
}

void test_reference_window_f32()
{
    printf("[TEST] Window vs scipy f32\n");
    kfr_f32 hann[9];
    kfr_f32 hamming[8];
    kfr_window_f32(KFR_WINDOW_HANN, 9, 0.f, 1, hann);
    kfr_window_f32(KFR_WINDOW_HAMMING, 8, 0.54f, 0, hamming);
    for (size_t i = 0; i < 9; i++)
        check_near("hann symmetric f32", i, hann[i], ref_hann_sym9[i], REF_TOL_F32);
    for (size_t i = 0; i < 8; i++)
        check_near("hamming periodic f32", i, hamming[i], ref_hamming_per8[i], REF_TOL_F32);
}

void test_reference_window_f64()
{
    printf("[TEST] Window vs scipy f64\n");
    kfr_f64 hann[9];
    kfr_f64 hamming[8];
    kfr_window_f64(KFR_WINDOW_HANN, 9, 0.0, 1, hann);
    kfr_window_f64(KFR_WINDOW_HAMMING, 8, 0.54, 0, hamming);
    for (size_t i = 0; i < 9; i++)
        check_near("hann symmetric f64", i, hann[i], ref_hann_sym9[i], REF_TOL_F64);
    for (size_t i = 0; i < 8; i++)
        check_near("hamming periodic f64", i, hamming[i], ref_hamming_per8[i], REF_TOL_F64);
}

int main()
{
    CHECK(KFR_HEADERS_VERSION <= kfr_version(), "Dynamic library is too old. At least %d required",
          KFR_HEADERS_VERSION);

    printf("[INFO] %s\n", kfr_version_string());

    test_memory();
    test_dft_f32();
    test_dft_f64();
    test_fir_f32();
    test_fir_f64();
    test_iir_f32();
    test_iir_f64();
    test_reference_iir_f32();
    test_reference_iir_f64();
    test_reference_fir_f32();
    test_reference_fir_f64();
    test_reference_window_f32();
    test_reference_window_f64();

    if (failures == 0)
        printf("[PASSED]\n");
    else
        printf("[FAILED] %d check(s)\n", failures);
    return failures;
}
