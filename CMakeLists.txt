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

if (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles" OR ${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles")
    if (CMAKE_BUILD_TYPE_INITIALIZED_TO_DEFAULT)
        set(CMAKE_BUILD_TYPE Release)
    endif ()
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_FLAGS --target=x86_64-w64-windows-gnu CACHE STRING "compile flags" FORCE)
    set(CMAKE_C_FLAGS --target=x86_64-w64-windows-gnu CACHE STRING "compile flags" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS --target=x86_64-w64-windows-gnu)
    set(CMAKE_SHARED_LINKER_FLAGS --target=x86_64-w64-windows-gnu)
    set(CMAKE_STATIC_LINKER_FLAGS --target=x86_64-w64-windows-gnu)
endif ()

project(kfr)

include(sources.cmake)

add_compile_options(-std=c++1y)

set(ALL_WARNINGS -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c99-extensions -Wno-padded)

add_compile_options(-march=native)

add_subdirectory(examples)
add_subdirectory(tests)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/svg)

