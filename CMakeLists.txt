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

if (CMAKE_VERSION VERSION_LESS "2.8.12")
    function(add_compile_options)
        add_definitions(${ARGN})
    endfunction(add_compile_options)
endif ()

#set(DISABLE_CLANG 1)

if(NOT CMAKE_CXX_COMPILER)

    if (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles"
        OR ${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles"
        OR ${CMAKE_GENERATOR} STREQUAL "Unix Makefiles"
        OR ${CMAKE_GENERATOR} STREQUAL "Ninja")

        # Find clang and set as c++ compiler for MinGW, MSYS and Unix
        # If clang is not found, leave default compiler (usually GCC)

        if(NOT DISABLE_CLANG)
            find_program(CLANG_CXX_PATH clang++-4.0)
            find_program(CLANG_CXX_PATH clang++-3.9)
            find_program(CLANG_CXX_PATH clang++-3.8)
            find_program(CLANG_CXX_PATH clang++-3.7)
            find_program(CLANG_CXX_PATH clang++-3.6)
            find_program(CLANG_CXX_PATH clang++)

            # If clang is found
            if(CLANG_CXX_PATH)
                set(CMAKE_CXX_COMPILER ${CLANG_CXX_PATH})
                message(STATUS "Clang found")
            else()
                message(STATUS "Clang not found")
            endif()

        else()
            message(STATUS "Clang disabled")
        endif()

    elseif (${CMAKE_GENERATOR} STREQUAL "Visual Studio 14 2015"
        OR ${CMAKE_GENERATOR} STREQUAL "Visual Studio 14 2015 Win64")

        # Find LLVM-vs2014 toolset for MSVC
        # If clang is not found, leave default Microsoft's cl.exe compiler

        if(NOT DISABLE_CLANG)
            file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test_toolset")

            # Check if toolset can be set
            execute_process(COMMAND "${CMAKE_COMMAND}" -G "Visual Studio 14 2015" -T LLVM-vs2014 "${CMAKE_SOURCE_DIR}/cmake/test_toolset" WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test_toolset" RESULT_VARIABLE TOOLSET_EXISTS OUTPUT_QUIET ERROR_QUIET)

            if(${TOOLSET_EXISTS} EQUAL 0)
                set(CMAKE_GENERATOR_TOOLSET LLVM-vs2014)
                message(STATUS "LLVM-vs2014 found")
            else()
                message(STATUS "LLVM-vs2014 not found")
            endif()
        else()
            message(STATUS "LLVM-vs2014 disabled")
        endif()

    endif()
endif()

if (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles"
    OR ${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles"
    OR ${CMAKE_GENERATOR} STREQUAL "Unix Makefiles"
    OR ${CMAKE_GENERATOR} STREQUAL "Ninja")
    
    if (CMAKE_CXX_COMPILER MATCHES "clang")
        if (WIN32)
            # On windows, clang requires --target
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=x86_64-w64-windows-gnu"  CACHE STRING "cxx compile flags" FORCE)
        endif()
    endif()
endif()

####################################

# Release build by default
if (CMAKE_BUILD_TYPE_INITIALIZED_TO_DEFAULT)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# Only Debug and Release builds
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "config types" FORCE)

# Insert common c++ flags here
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS}" CACHE STRING "compile flags" FORCE)

project(kfr CXX)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} ${CMAKE_CXX_COMPILER} ")

# Binary output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin/Debug)

# Include list of source files
include(sources.cmake)

if (IOS)
    set(STD_LIB)
else ()
    set(STD_LIB stdc++)
endif ()

if (NOT MSVC)
    # Enable C++14, disable exceptions and rtti
    add_compile_options(-std=c++1y -fno-exceptions -fno-rtti )
    if (NOT ARCH_FLAGS)
        add_compile_options(-march=native)
        if(WIN32)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign -fno-asynchronous-unwind-tables")
        endif()
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
    endif ()
    link_libraries(${STD_LIB} pthread m)
else ()
    # Disable exceptions
    add_compile_options(/EHsc /D_HAS_EXCEPTIONS=0 /D_CRT_SECURE_NO_WARNINGS=1)
    add_compile_options(/arch:AVX)
endif ()

if ("${CMAKE_GENERATOR_TOOLSET}" STREQUAL "LLVM-vs2014")
    add_compile_options( -Wno-unused-command-line-argument )
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tests/cmake/")

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/svg)

add_subdirectory(examples)
add_subdirectory(tests)
