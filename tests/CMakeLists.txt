# Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
# This file is part of KFR
#
# KFR is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# KFR is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with KFR.

cmake_minimum_required(VERSION 3.10)

add_definitions(-DKFR_TESTING=1)
add_definitions(-DKFR_SRC_DIR=\"${CMAKE_SOURCE_DIR}\")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fdiagnostics-absolute-paths)
endif ()

option(KFR_ENABLE_COVERAGE "Enable coverage reporting" OFF)

if (KFR_ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    endif ()
endif ()

if (KFR_ENABLE_DFT)
    add_definitions(-DHAVE_DFT)
endif ()

if (MSVC)
    link_libraries(-DEBUG)
else ()
    add_compile_options(-g)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-parentheses)
endif ()

# Binary output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tests/cmake/")

if (KFR_ENABLE_CAPI_BUILD)
    add_executable(capi_test capi_test.cpp)
    target_include_directories(capi_test
                               PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(capi_test PUBLIC kfr_capi)
endif ()

if (KFR_ENABLE_ASMTEST)
    add_executable(asm_test asm_test.cpp)
    target_link_libraries(asm_test kfr)
    target_set_arch(asm_test PRIVATE avx2)
    target_compile_definitions(asm_test PRIVATE KFR_SHOW_NOT_OPTIMIZED)
    target_compile_definitions(asm_test PRIVATE KFR_FUNCTION_IS_INTRINSIC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(asm_test PRIVATE -fno-stack-protector)
    endif ()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(asm_test PRIVATE -GS-)
        target_compile_options(asm_test PRIVATE -Gs16384)
    endif ()

    add_custom_command(
        TARGET asm_test
        POST_BUILD
        COMMAND objconv -fyasm $<TARGET_FILE:asm_test>)
endif ()

if (X86)
    if (MSVC AND NOT CLANG)
        add_executable(multiarch multiarch.cpp multiarch_fir_sse2.cpp
                                 multiarch_fir_avx.cpp)
        set_source_files_properties(multiarch_fir_sse2.cpp
                                    PROPERTIES COMPILE_FLAGS /arch:SSE2)
        set_source_files_properties(multiarch_fir_avx.cpp
                                    PROPERTIES COMPILE_FLAGS /arch:AVX)
    else ()
        add_executable(multiarch multiarch.cpp multiarch_fir_sse2.cpp
                                 multiarch_fir_avx.cpp)
        set_source_files_properties(
            multiarch_fir_sse2.cpp PROPERTIES COMPILE_FLAGS
                                              "-mno-avx -mno-sse3 -msse2")
        set_source_files_properties(multiarch_fir_avx.cpp
                                    PROPERTIES COMPILE_FLAGS "-mavx -mno-avx2")
    endif ()
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

if (KFR_ENABLE_DFT)
    list(APPEND ALL_TESTS_CPP dft_test.cpp)

    add_executable(dft_test dft_test.cpp)
endif ()

if (KFR_REGENERATE_TESTS)
    find_package(MPFR)
    find_package(GMP)

    if (MPFR_FOUND AND GMP_FOUND)
        add_executable(generate_data generate_data.cpp)
        target_link_libraries(generate_data kfr)
        target_include_directories(generate_data PRIVATE ${MPFR_INCLUDE_DIR}
                                                         ${GMP_INCLUDE_DIR})
        target_link_libraries(generate_data ${MPFR_LIBRARIES} ${GMP_LIBRARIES})
        if (KFR_REGENERATE_TESTS)
            add_custom_command(
                TARGET generate_data
                POST_BUILD
                COMMENT "Generating tests..."
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/data
                COMMAND generate_data)
        endif ()
    endif ()
endif ()

add_executable(all_tests all_tests.cpp ${ALL_TESTS_CPP})
target_compile_definitions(all_tests PRIVATE KFR_NO_MAIN)
target_link_libraries(all_tests kfr use_arch)
if (KFR_ENABLE_DFT)
    target_link_libraries(all_tests kfr_dft)
    target_link_libraries(dft_test kfr_dft)
endif ()
target_link_libraries(all_tests kfr_io)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(all_tests PRIVATE /Zc:lambda)
endif ()

function (add_x86_test ARCH)
    set(NAME ${ARCH})

    add_executable(all_tests_${NAME} all_tests.cpp ${ALL_TESTS_CPP}
                                     ${KFR_IO_SRC})
    if (KFR_ENABLE_DFT)
        target_sources(all_tests_${NAME} PRIVATE ${KFR_DFT_SRC})
    endif ()
    target_link_libraries(all_tests_${NAME} kfr)
    target_set_arch(all_tests_${NAME} PRIVATE ${ARCH})
    target_compile_definitions(all_tests_${NAME} PRIVATE KFR_NO_MAIN)
    target_compile_definitions(all_tests_${NAME} PUBLIC KFR_ENABLE_FLAC=1)

    if (KFR_ARCH_TESTS_MULTI)
        add_library(all_tests_multiarch_${NAME} STATIC ${ALL_TESTS_MERGED_CPP}
                                                       ${KFR_IO_SRC})
        if (KFR_ENABLE_DFT)
            target_sources(all_tests_multiarch_${NAME} PRIVATE ${KFR_DFT_SRC})
        endif ()
        target_link_libraries(all_tests_multiarch_${NAME} kfr)
        target_set_arch(all_tests_multiarch_${NAME} PRIVATE ${ARCH})
        target_compile_definitions(all_tests_multiarch_${NAME}
                                   PRIVATE KFR_NO_MAIN)
        target_compile_definitions(all_tests_multiarch_${NAME}
                                   PUBLIC KFR_ENABLE_FLAC=1)
    endif ()

endfunction ()

if (KFR_ARCH_TESTS AND KFR_ARCH_TESTS STREQUAL "ON")
    set(ARCH_LIST
        generic
        sse2
        ssse3
        sse42
        avx
        avx2
        avx512)
else ()
    string(REPLACE "," ";" ARCH_LIST "${KFR_ARCH_TESTS}")
endif ()

if (MSVC AND NOT CLANG)
    list(REMOVE_ITEM ARCH_LIST generic)
endif ()

if (KFR_ARCH_TESTS)

    foreach (A IN LISTS ARCH_LIST)
        add_x86_test(${A})
    endforeach ()

    if (KFR_ARCH_TESTS_MULTI)
        add_executable(all_tests_multiarch all_tests.cpp)
        target_compile_definitions(all_tests_multiarch PRIVATE KFR_MULTI_ARCH)
        foreach (A IN LISTS ARCH_LIST)
            target_link_libraries(all_tests_multiarch all_tests_multiarch_${A})
        endforeach ()
    endif ()
endif ()

if (KFR_USE_SDE)
    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        find_program(SDE NAMES "sde64" "sde")
    else ()
        find_program(SDE NAMES "sde")
    endif ()
    set(EMULATOR "${SDE}")
    list(APPEND EMULATOR "-skx")
    list(APPEND EMULATOR "--")
elseif (NOT EMULATOR)
    set(EMULATOR "")
    set(SDE "")
endif ()

set(SDE_ARCH_generic -p4p)
set(SDE_ARCH_sse2 -p4p)
set(SDE_ARCH_sse3 -p4p)
set(SDE_ARCH_ssse3 -mrm)
set(SDE_ARCH_sse41 -pnr)
set(SDE_ARCH_sse42 -nhm)
set(SDE_ARCH_avx -snb)
set(SDE_ARCH_avx2 -hsw)
set(SDE_ARCH_avx512 -skx)

if (NOT KFR_SKIP_TESTS)
    enable_testing()

    if (X86)
        add_test(NAME multiarch COMMAND ${EMULATOR}
                                        ${PROJECT_BINARY_DIR}/bin/multiarch)

        if (KFR_ENABLE_CAPI_BUILD)
            add_test(NAME capi_test COMMAND ${EMULATOR}
                                            ${PROJECT_BINARY_DIR}/bin/capi_test)
        endif ()
    endif ()

    if (KFR_ARCH_TESTS)
        foreach (A IN LISTS ARCH_LIST)
            if (KFR_USE_SDE)
                add_test(NAME all_tests_${A}
                         COMMAND ${SDE} ${SDE_ARCH_${A}} -chip_check_exe_only
                                 -- ${PROJECT_BINARY_DIR}/bin/all_tests_${A})
            else ()
                add_test(NAME all_tests_${A}
                         COMMAND ${EMULATOR}
                                 ${PROJECT_BINARY_DIR}/bin/all_tests_${A})
            endif ()
        endforeach ()
    endif ()
    if (KFR_USE_SDE)
        add_test(NAME all_tests
                 COMMAND ${SDE} ${SDE_ARCH_${KFR_ARCH}} -chip_check_exe_only --
                         ${PROJECT_BINARY_DIR}/bin/all_tests)
        add_test(NAME all_tests_on_avx512
                 COMMAND ${SDE} ${SDE_ARCH_avx512} -chip_check_exe_only --
                         ${PROJECT_BINARY_DIR}/bin/all_tests)
    else ()
        add_test(NAME all_tests COMMAND ${PROJECT_BINARY_DIR}/bin/all_tests)
    endif ()
endif ()
