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

set(OPT_BITNESS "") # cmake -DOPT_BITNESS="-m32" or -m64
set(OPT_STATIC "") # cmake -DOPT_STATIC="-static"

if (CMAKE_BUILD_TYPE_INITIALIZED_TO_DEFAULT)
    set(CMAKE_BUILD_TYPE Release)
endif ()
if (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles" OR ${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles")
    set(OPT_TARGET "--target=x86_64-w64-windows-gnu")
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER clang)
else ()
    set(OPT_TARGET "") # default target
endif ()

if (${CMAKE_GENERATOR} STREQUAL "Visual Studio 14 2015")
    set(CMAKE_GENERATOR_TOOLSET LLVM-vs2014)
endif ()
set(CMAKE_CXX_FLAGS "${OPT_TARGET} ${OPT_BITNESS} ${OPT_STATIC}" CACHE STRING "compile flags" FORCE)
set(CMAKE_C_FLAGS "${OPT_TARGET} ${OPT_BITNESS} ${OPT_STATIC}" CACHE STRING "compile flags" FORCE)
#set(CMAKE_EXE_LINKER_FLAGS "${OPT_TARGET} ${OPT_BITNESS}")
#set(CMAKE_SHARED_LINKER_FLAGS "${OPT_TARGET} ${OPT_BITNESS}")
#set(CMAKE_STATIC_LINKER_FLAGS "${OPT_TARGET} ${OPT_BITNESS}")

project(kfr)

include(sources.cmake)

set(ALL_WARNINGS -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c99-extensions -Wno-padded)

if (IOS)
    set(STD_LIB)
else ()
    set(STD_LIB stdc++)
endif ()

if (NOT MSVC)
    add_compile_options(-std=c++1y)
else ()
    add_compile_options(/EHsc /D_HAS_EXCEPTIONS=0)
endif ()

add_subdirectory(examples)
add_subdirectory(tests)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/svg)

