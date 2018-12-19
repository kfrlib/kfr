# Copyright (C) 2016 D Levin (http://www.kfrlib.com)
# This file is part of KFR
#
# KFR is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# KFR is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with KFR.


cmake_minimum_required(VERSION 3.0)

add_definitions(-DKFR_TESTING=1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tests/cmake/")

if (NOT ARM)
    if(MSVC AND NOT CLANG)
        add_executable(multiarch multiarch.cpp multiarch_fir_sse2.cpp multiarch_fir_avx.cpp)
        set_source_files_properties(multiarch_fir_sse2.cpp PROPERTIES COMPILE_FLAGS /arch:SSE2)
        set_source_files_properties(multiarch_fir_avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX)
    else()
        add_executable(multiarch multiarch.cpp multiarch_fir_sse2.cpp multiarch_fir_avx.cpp)
        set_source_files_properties(multiarch_fir_sse2.cpp PROPERTIES COMPILE_FLAGS "-mno-avx -mno-sse3 -msse2")
        set_source_files_properties(multiarch_fir_avx.cpp PROPERTIES COMPILE_FLAGS "-mavx -mno-avx2")
    endif()
    target_link_libraries(multiarch kfr)
endif ()

find_package(MPFR)
find_package(GMP)

set(ALL_TESTS_CPP
    all_tests.cpp
    base_test.cpp
    complex_test.cpp
    dft_test.cpp
    dsp_test.cpp
    expression_test.cpp
    intrinsic_test.cpp io_test.cpp resampler_test.cpp)

if (MPFR_FOUND AND GMP_FOUND)
    list(APPEND ALL_TESTS_CPP transcendental_test.cpp)
else ()
    message(STATUS "MPFR is not found. Skipping transcendental_test")
endif ()

add_executable(all_tests ${ALL_TESTS_CPP})
target_compile_definitions(all_tests PRIVATE KFR_NO_MAIN)
target_link_libraries(all_tests kfr kfr_dft)
target_link_libraries(all_tests kfr kfr_dft kfr_io)

if (MPFR_FOUND AND GMP_FOUND)
    add_definitions(-DHAVE_MPFR)
    include_directories(${MPFR_INCLUDE_DIR} ${GMP_INCLUDE_DIR})
    target_link_libraries(all_tests ${MPFR_LIBRARIES} ${GMP_LIBRARIES})
endif ()

function(add_x86_test NAME FLAGS)
    separate_arguments(FLAGS)
    add_executable(all_tests_${NAME} ${ALL_TESTS_CPP}  ${KFR_DFT_SRC} ${KFR_IO_SRC})
    target_compile_options(all_tests_${NAME} PRIVATE ${FLAGS})
    target_compile_definitions(all_tests_${NAME} PRIVATE KFR_NO_MAIN)
    target_link_libraries(all_tests_${NAME} kfr)
    target_compile_definitions(all_tests_${NAME} PUBLIC KFR_ENABLE_FLAC=1)
    if (MPFR_FOUND AND GMP_FOUND)
        target_link_libraries(all_tests_${NAME} ${MPFR_LIBRARIES} ${GMP_LIBRARIES})
    endif ()
endfunction()

if (ARCH_TESTS)
    set (ARCH_RESET "-march=x86-64 -mno-sse3 -mno-ssse3 -mno-sse4.1 -mno-sse4.2 -mno-avx -mno-avx2 -mno-fma -mno-avx512f -mno-avx512cd -mno-avx512bw -mno-avx512dq -mno-avx512vl")
    add_x86_test(generic "${ARCH_RESET} -DCMT_FORCE_GENERIC_CPU")
    add_x86_test(sse2 "${ARCH_RESET} -msse2")
    add_x86_test(sse3 "${ARCH_RESET} -msse3 -mno-avx")
    add_x86_test(ssse3 "${ARCH_RESET} -mssse3 -mno-avx")
    add_x86_test(sse41 "${ARCH_RESET} -msse4.1 -mno-avx")
    add_x86_test(avx "${ARCH_RESET} -msse4.1 -mavx")
    add_x86_test(avx2 "${ARCH_RESET} -msse4.1 -mavx2 -mfma")
    add_x86_test(avx512 "${ARCH_RESET} -msse4.1 -mavx2 -mfma -mavx512f -mavx512cd -mavx512bw -mavx512dq -mavx512vl")
endif()

if(USE_SDE)
    find_program(EMULATOR "sde")
    list(APPEND EMULATOR "-skx")
    list(APPEND EMULATOR "--")
elseif (ARM)
    find_program(EMULATOR "qemu-arm")
else ()
    set(EMULATOR "")
endif ()

if (NOT IOS)
    enable_testing()

    if (NOT ARM)
        add_test(NAME multiarch
                COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/multiarch)
    endif ()

    add_test(NAME all_tests
            COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests)

    if (ARCH_TESTS)
        add_test(NAME generic COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests_generic )
        add_test(NAME sse2    COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests_sse2    )
        add_test(NAME sse3    COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests_sse3    )
        add_test(NAME ssse3   COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests_ssse3   )
        add_test(NAME sse41   COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests_sse41   )
        add_test(NAME avx     COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests_avx     )
        add_test(NAME avx2    COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests_avx2    )
        add_test(NAME avx512  COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/bin/all_tests_avx512  )
    endif ()
endif ()
