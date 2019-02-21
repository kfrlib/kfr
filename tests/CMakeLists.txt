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


cmake_minimum_required(VERSION 3.1)

add_definitions(-DKFR_TESTING=1)
add_definitions(-DKFR_SRC_DIR=\"${CMAKE_SOURCE_DIR}\")

# Binary output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tests/cmake/")

if (ENABLE_ASMTEST)
    add_executable(asm_test asm_test.cpp)
    target_link_libraries(asm_test kfr)
    target_set_arch(asm_test PRIVATE avx)
    target_compile_definitions(asm_test PRIVATE KFR_SHOW_NOT_OPTIMIZED)

    add_custom_command(TARGET asm_test POST_BUILD COMMAND objconv -fyasm $<TARGET_FILE:asm_test>)
endif()

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

set(ALL_TESTS_CPP
        base_test.cpp
        complex_test.cpp
        dsp_test.cpp
        expression_test.cpp
        intrinsic_test.cpp
        io_test.cpp
        ${KFR_UNITTEST_SRC})

# set(ALL_TESTS_MERGED_CPP all_tests_merged.cpp)

if (ENABLE_DFT)
    list(APPEND ALL_TESTS_CPP dft_test.cpp)
endif ()

find_package(MPFR)
find_package(GMP)

if (MPFR_FOUND AND GMP_FOUND)
    message(STATUS "MPFR is found")
    add_executable(generate_data generate_data.cpp)
    target_link_libraries(generate_data kfr)
    target_include_directories(generate_data PRIVATE ${MPFR_INCLUDE_DIR} ${GMP_INCLUDE_DIR})
    target_link_libraries(generate_data ${MPFR_LIBRARIES} ${GMP_LIBRARIES})
    if (REGENERATE_TESTS)
        add_custom_command(TARGET generate_data POST_BUILD
                COMMENT "Generating tests..."
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/data
                COMMAND generate_data)
    endif ()
endif ()

add_executable(all_tests all_tests.cpp ${ALL_TESTS_CPP})
target_compile_definitions(all_tests PRIVATE KFR_NO_MAIN)
target_link_libraries(all_tests kfr use_arch)
if (ENABLE_DFT)
    target_link_libraries(all_tests kfr_dft)
endif ()
target_link_libraries(all_tests kfr_io)

function(add_x86_test ARCH)
    set(NAME ${ARCH})

    add_executable(all_tests_${NAME} all_tests.cpp ${ALL_TESTS_CPP}  ${KFR_IO_SRC})
    if (ENABLE_DFT)
        target_sources(all_tests_${NAME} PRIVATE ${KFR_DFT_SRC})
    endif ()
    target_link_libraries(all_tests_${NAME} kfr)
    target_set_arch(all_tests_${NAME} PRIVATE ${ARCH})
    target_compile_definitions(all_tests_${NAME} PRIVATE KFR_NO_MAIN)
    target_compile_definitions(all_tests_${NAME} PUBLIC KFR_ENABLE_FLAC=1)

    if (ARCH_TESTS_MULTI)
        add_library(all_tests_multiarch_${NAME} STATIC ${ALL_TESTS_MERGED_CPP}  ${KFR_IO_SRC})
        if (ENABLE_DFT)
            target_sources(all_tests_multiarch_${NAME} PRIVATE ${KFR_DFT_SRC})
        endif ()
        target_link_libraries(all_tests_multiarch_${NAME} kfr)
        target_set_arch(all_tests_multiarch_${NAME} PRIVATE ${ARCH})
        target_compile_definitions(all_tests_multiarch_${NAME} PRIVATE KFR_NO_MAIN)
        target_compile_definitions(all_tests_multiarch_${NAME} PUBLIC KFR_ENABLE_FLAC=1)
    endif ()

endfunction()

if (ARCH_TESTS)
    if (NOT MSVC OR CLANG)
        add_x86_test(generic)
    endif ()
    add_x86_test(sse2)
    add_x86_test(sse3)
    add_x86_test(ssse3)
    add_x86_test(sse41)
    add_x86_test(avx)
    add_x86_test(avx2)
    add_x86_test(avx512)

    if (ARCH_TESTS_MULTI)
        add_executable(all_tests_multiarch all_tests.cpp)
        target_compile_definitions(all_tests_multiarch PRIVATE KFR_MULTI_ARCH)
        target_link_libraries(all_tests_multiarch
                all_tests_multiarch_sse2
                all_tests_multiarch_sse3
                all_tests_multiarch_ssse3
                all_tests_multiarch_sse41
                all_tests_multiarch_avx
                all_tests_multiarch_avx2
                all_tests_multiarch_avx512
                )
    endif ()
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
