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


cmake_minimum_required(VERSION 2.8)

set(TEST_SRC
    testo/testo.hpp
)

if (NOT MSVC)
    add_compile_options(-fno-exceptions -fno-rtti -ftemplate-backtrace-limit=0)
    link_libraries(${STD_LIB} pthread m)
endif ()

include_directories(../include)

if (NOT ARM)
    add_executable(multiarch multiarch.cpp multiarch_fir_sse2.cpp multiarch_fir_avx.cpp ${KFR_SRC})
    set_source_files_properties(multiarch_fir_sse2.cpp PROPERTIES COMPILE_FLAGS "-mno-avx -mno-sse3 -msse2")
    set_source_files_properties(multiarch_fir_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
endif ()

if (NOT MSVC)
    if (NOT ARCH_FLAGS)
        add_compile_options(-march=native)
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
    endif ()
else ()
    add_compile_options(/arch:AVX)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tests/cmake/")

find_package(MPFR)

add_executable(intrinsic_test intrinsic_test.cpp ${KFR_SRC} ${TEST_SRC})
add_executable(dft_test dft_test.cpp ${KFR_SRC} ${TEST_SRC})
if (MPFR_FOUND)
    include_directories(${MPFR_INCLUDE_DIR})
    add_executable(transcendental_test transcendental_test.cpp ${KFR_SRC} ${TEST_SRC})
    target_link_libraries(transcendental_test ${MPFR_LIBRARIES})
endif ()
add_executable(dsp_test dsp_test.cpp ${KFR_SRC} ${TEST_SRC})
add_executable(empty_test empty_test.cpp ${KFR_SRC} ${TEST_SRC})
add_executable(complex_test complex_test.cpp ${KFR_SRC} ${TEST_SRC})
add_executable(base_test base_test.cpp ${KFR_SRC} ${TEST_SRC})
add_executable(expression_test expression_test.cpp ${KFR_SRC} ${TEST_SRC})

if (ARM)
    find_program(EMULATOR "qemu-arm")
else ()
    set(EMULATOR "")
endif ()

if (NOT IOS)
    enable_testing()

    add_test(NAME base_test
            COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/tests/base_test)
    add_test(NAME intrinsic_test
            COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/tests/intrinsic_test)
    add_test(NAME dsp_test
            COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/tests/dsp_test)
    if (MPFR_FOUND)
        add_test(NAME transcendental_test
                COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/tests/transcendental_test)
    endif ()
    add_test(NAME complex_test
            COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/tests/complex_test)
    add_test(NAME expression_test
            COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/tests/expression_test)

    if (NOT ARM)
        add_test(NAME multiarch
                COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/tests/multiarch)
    endif ()
    add_test(NAME dft_test
            COMMAND ${EMULATOR} ${PROJECT_BINARY_DIR}/tests/dft_test)
endif ()
