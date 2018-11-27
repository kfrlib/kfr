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

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS}" CACHE STRING "compile flags" FORCE)

project(kfr CXX)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} ${CMAKE_CXX_COMPILER} ")

message(STATUS CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR})
if (MSVC)
    message(STATUS MSVC)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(CLANG 1)
else()
    set(CLANG 0)
endif()

# Include list of source files
include(sources.cmake)

add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)

option(ENABLE_TESTS "Enable tests and examples. This changes many compiler flags" OFF)

set(KFR_DFT_SRC
        ${CMAKE_SOURCE_DIR}/include/kfr/dft/impl/dft-src.cpp
        ${CMAKE_SOURCE_DIR}/include/kfr/dft/dft_c.h
        ${CMAKE_SOURCE_DIR}/include/kfr/dft/impl/dft-impl-f32.cpp
        ${CMAKE_SOURCE_DIR}/include/kfr/dft/impl/dft-impl-f64.cpp
        ${CMAKE_SOURCE_DIR}/include/kfr/dft/impl/convolution-impl.cpp)

if (ENABLE_TESTS)

    if (IOS)
        set(STD_LIB)
    else ()
        set(STD_LIB stdc++)
    endif ()

    # Binary output directories
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    if (NOT MSVC OR CLANG)
        # Enable C++14, disable exceptions and rtti
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-std=gnu++1y)
        else ()
            add_compile_options(-std=c++1y)
        endif ()
        add_compile_options(-fno-exceptions -fno-rtti )
        if (NOT ARCH_FLAGS)
            add_compile_options(-march=native)
            message(STATUS "Building for native cpu")
            if(WIN32)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign -fno-asynchronous-unwind-tables")
            endif()
        else ()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
        endif ()
        if(NOT MSVC)
            link_libraries(${STD_LIB} pthread m)
        endif()
    else ()
        # Disable exceptions
        add_compile_options(/EHsc /D_HAS_EXCEPTIONS=0 /D_CRT_SECURE_NO_WARNINGS=1)
        add_compile_options(/arch:AVX)
    endif ()

    add_subdirectory(examples)
    add_subdirectory(tests)
endif ()

add_library(kfr INTERFACE)
target_sources(kfr INTERFACE ${KFR_SRC})
target_include_directories(kfr INTERFACE include)

add_library(kfr_dft ${KFR_DFT_SRC})
target_link_libraries(kfr_dft kfr)
