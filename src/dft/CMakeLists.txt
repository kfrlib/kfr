cmake_minimum_required(VERSION 3.12)

set(DFT_LIBS)

if (KFR_ENABLE_DFT_MULTIARCH)
    add_library(kfr_dft INTERFACE)
    add_library(kfr_dft_all INTERFACE)
    target_link_libraries(kfr_dft INTERFACE kfr kfr_dft_all)
    target_compile_definitions(
        kfr_dft
        INTERFACE -DKFR_DFT_MULTI=1
                  -DCMT_MULTI=1
                  -DCMT_MULTI_ENABLED_SSE2=1
                  -DCMT_MULTI_ENABLED_SSE41=1
                  -DCMT_MULTI_ENABLED_AVX=1
                  -DCMT_MULTI_ENABLED_AVX2=1
                  -DCMT_MULTI_ENABLED_AVX512=1)

    add_arch_library(kfr_dft sse2 "${KFR_DFT_SRC}" "")
    add_arch_library(kfr_dft sse41 "${KFR_DFT_SRC}" "")
    add_arch_library(kfr_dft avx "${KFR_DFT_SRC}" "")
    add_arch_library(kfr_dft avx2 "${KFR_DFT_SRC}" "")
    add_arch_library(kfr_dft avx512 "${KFR_DFT_SRC}" "")
    list(
        APPEND
        DFT_LIBS
        kfr_dft_sse2
        kfr_dft_sse41
        kfr_dft_avx
        kfr_dft_avx2
        kfr_dft_avx512)

    link_as_whole(kfr_dft_all INTERFACE kfr_dft_sse2)

else ()
    add_library(kfr_dft ${KFR_DFT_SRC})
    target_link_libraries(kfr_dft kfr use_arch)
    if (KFR_ENABLE_DFT_NP)
        target_compile_definitions(kfr_dft PUBLIC -DKFR_DFT_NPo2)
    else ()
        target_compile_definitions(kfr_dft PUBLIC -DKFR_DFT_NO_NPo2)
    endif ()
    list(APPEND DFT_LIBS kfr_dft)
endif ()

function (dft_compile_options LIB)
    if (MSVC AND CLANG)
        target_compile_options(${LIB} PRIVATE SHELL:-Xclang -ffp-contract=fast
                                              -Xclang -O3)
    else ()
        target_compile_options(${LIB} PRIVATE -ffp-contract=fast -O3)
    endif ()
endfunction ()

foreach (LIB IN LISTS DFT_LIBS)
    dft_compile_options(${LIB})
endforeach ()

if (KFR_INSTALL_LIBRARIES)
    if (KFR_ENABLE_DFT_MULTIARCH)
        install(
            TARGETS kfr_dft_sse2 kfr_dft_sse41 kfr_dft_avx kfr_dft_avx2
                    kfr_dft_avx512
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)
    else ()
        install(
            TARGETS kfr_dft
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)
    endif ()
endif ()
