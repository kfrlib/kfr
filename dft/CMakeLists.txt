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

if (MSVC)
    set(LIBABI "vc${MSVC_TOOLSET_VERSION}")
elseif (WIN32)
    set(LIBABI "mingw")
elseif (APPLE)
    set(LIBABI "apple")
elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    set(LIBABI "linux")
else ()
    set(LIBABI "unknown")
    message(WARNING "KFR DFT: Unknown ABI")
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIBABI "${LIBABI}-x64")
else ()
    set(LIBABI "${LIBABI}-x32")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(LIBABI "${LIBABI}-d")
endif ()

message(STATUS "LIBABI = ${LIBABI}")

add_custom_target(kfr_multiarch_dft)

function(add_dft_library ARCH)
    add_library(kfr_dft_${LIBABI}_${ARCH} STATIC ${KFR_DFT_SRC})
    target_link_libraries(kfr_dft_${LIBABI}_${ARCH} kfr)
    target_set_arch(kfr_dft_${LIBABI}_${ARCH} PUBLIC ${ARCH})
    target_compile_options(kfr_dft_${LIBABI}_${ARCH} PRIVATE -Xclang -ffast-math)
    add_dependencies(kfr_multiarch_dft kfr_dft_${LIBABI}_${ARCH})
endfunction()

add_dft_library(sse2)
add_dft_library(sse41)
add_dft_library(avx)
add_dft_library(avx2)
add_dft_library(avx512)