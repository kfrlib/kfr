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
    CHECK(kfr_allocated_size(d) == 256, "kfr_allocated_size: wrong size: %zu", kfr_allocated_size(d));
    d = (uint8_t*)(kfr_reallocate(d, 512));
    CHECK(kfr_allocated_size(d) == 512, "kfr_allocated_size: wrong size: %zu", kfr_allocated_size(d));
    for (size_t i = 0; i < 256; i++)
    {
        CHECK(d[i] == i, "kfr_reallocate: data lost after reallocation\n");
    }
    kfr_deallocate(d);

    void* page = kfr_allocate_aligned(4096, 4096);
    CHECK(((uintptr_t)page & 0xFFF) == 0, "kfr_allocate_aligned: wrong alignment: 0x%zx",
          ((uintptr_t)page & 0xFFF));

    kfr_deallocate(page);
}

#define DFT_SIZE 32

void test_dft_f32()
{
    printf("[TEST] DFT f32\n");
    const float eps        = 0.00001f;
    KFR_DFT_PLAN_F32* plan = kfr_dft_create_plan_f32(DFT_SIZE);
    // kfr_dft_dump_f32(plan);
    kfr_f32 buf[DFT_SIZE * 2];
    for (int i = 0; i < DFT_SIZE; i++)
    {
        buf[i * 2 + 0] = (float)(i) / DFT_SIZE;
        buf[i * 2 + 1] = (float)(-i) / DFT_SIZE;
    }
    uint8_t* tmp = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f32(plan));
    kfr_dft_execute_f32(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f32(plan, buf, buf, tmp);
    kfr_dft_delete_plan_f32(plan);
    kfr_deallocate(tmp);

    for (int i = 0; i < DFT_SIZE; i++)
    {
        CHECK(fabsf(buf[i * 2 + 0] - (float)(i)) < eps, "DFT: wrong result at %d: re = %f", i,
              buf[i * 2 + 0]);
        CHECK(fabsf(buf[i * 2 + 1] - (float)(-i)) < eps, "DFT: wrong result at %d: im = %f", i,
              buf[i * 2 + 1]);
    }
}

void test_dft_f64()
{
    printf("[TEST] DFT f64\n");
    const float eps        = 0.00000001;
    KFR_DFT_PLAN_F64* plan = kfr_dft_create_plan_f64(DFT_SIZE);
    // kfr_dft_dump_f64(plan);
    kfr_f64 buf[DFT_SIZE * 2];
    for (int i = 0; i < DFT_SIZE; i++)
    {
        buf[i * 2 + 0] = (double)(i) / DFT_SIZE;
        buf[i * 2 + 1] = (double)(-i) / DFT_SIZE;
    }
    uint8_t* tmp = (uint8_t*)kfr_allocate(kfr_dft_get_temp_size_f64(plan));
    kfr_dft_execute_f64(plan, buf, buf, tmp);
    kfr_dft_execute_inverse_f64(plan, buf, buf, tmp);
    kfr_dft_delete_plan_f64(plan);
    kfr_deallocate(tmp);

    for (int i = 0; i < DFT_SIZE; i++)
    {
        CHECK(fabs(buf[i * 2 + 0] - (double)(i)) < eps, "DFT: wrong result at %d: re = %f", i,
              buf[i * 2 + 0]);
        CHECK(fabs(buf[i * 2 + 1] - (double)(-i)) < eps, "DFT: wrong result at %d: im = %f", i,
              buf[i * 2 + 1]);
    }
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

    if (failures == 0)
        printf("[PASSED]\n");
    else
        printf("[FAILED] %d check(s)\n", failures);
    return failures;
}
