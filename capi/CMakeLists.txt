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

if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

set(CMAKE_CXX_VISIBILITY_PRESET "default")
set(CMAKE_C_VISIBILITY_PRESET "default")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fdiagnostics-absolute-paths)
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif ()

if (APPLE)
    add_compile_options(-mmacosx-version-min=10.9)
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib)

add_library(kfr_capi_all INTERFACE)
target_link_libraries(kfr_capi_all INTERFACE kfr)
if (NOT WIN32)
    add_library(kfr_capi_all_pic INTERFACE)
    target_link_libraries(kfr_capi_all_pic INTERFACE kfr)
endif ()

function (add_c_library ARCH)
    add_library(
        kfr_capi_${ARCH} STATIC
        ${KFR_DFT_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/kfr/dsp/impl/dsp-impl.cpp)
    target_link_libraries(kfr_capi_${ARCH} kfr)
    target_set_arch(kfr_capi_${ARCH} PRIVATE ${ARCH})
    target_compile_options(kfr_capi_${ARCH} PRIVATE "${CLANG_ARG_PREFIX}-ffp-contract=fast")
    target_link_libraries(kfr_capi_all INTERFACE kfr_capi_${ARCH})

    if (NOT WIN32)
        add_library(
            kfr_capi_${ARCH}_pic STATIC
            ${KFR_DFT_SRC}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kfr/dsp/impl/dsp-impl.cpp)
        set_property(TARGET kfr_capi_${ARCH}_pic
                     PROPERTY POSITION_INDEPENDENT_CODE 1)
        target_link_libraries(kfr_capi_${ARCH}_pic kfr)
        target_set_arch(kfr_capi_${ARCH}_pic PRIVATE ${ARCH})
        target_compile_options(kfr_capi_${ARCH}_pic PRIVATE "${CLANG_ARG_PREFIX}-ffp-contract=fast")

        target_link_libraries(kfr_capi_all_pic INTERFACE kfr_capi_${ARCH}_pic)
    endif ()
endfunction ()

add_library(kfr_capi SHARED ${PROJECT_SOURCE_DIR}/capi/capi.cpp)

add_c_library(sse2)
add_c_library(sse41)
add_c_library(avx)
add_c_library(avx2)
add_c_library(avx512)

target_compile_definitions(
    kfr_capi
    PRIVATE -DKFR_DFT_MULTI=1
            -DCMT_MULTI=1
            -DCMT_MULTI_ENABLED_SSE2=1
            -DCMT_MULTI_ENABLED_SSE41=1
            -DCMT_MULTI_ENABLED_AVX=1
            -DCMT_MULTI_ENABLED_AVX2=1
            -DCMT_MULTI_ENABLED_AVX512=1
            -DKFR_BUILDING_DLL=1)

target_set_arch(kfr_capi PRIVATE sse2)

if (WIN32)
    target_link_libraries(kfr_capi PRIVATE kfr kfr_capi_all)
else ()
    target_link_libraries(kfr_capi PRIVATE kfr kfr_capi_all_pic)

    if (APPLE)
        message(
            STATUS
                "Minimum macOS version is set to ${CMAKE_OSX_DEPLOYMENT_TARGET}"
        )
        message(STATUS "Set CMAKE_OSX_DEPLOYMENT_TARGET variable to change")
    else ()
        set_property(
            TARGET kfr_capi APPEND
            PROPERTY LINK_LIBRARIES
                     -nodefaultlibs
                     -Wl,-Bdynamic
                     -lm
                     -lc
                     -Wl,-Bstatic
                     -lstdc++
                     -lgcc
                     -s)
    endif ()
endif ()
