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

function(add_dft_library ARCH)
    add_library(kfr_dft_${ARCH} STATIC ${KFR_DFT_SRC})
    target_link_libraries(kfr_dft_${ARCH} kfr)
    target_set_arch(kfr_dft_${ARCH} INTERFACE ${ARCH})
    target_compile_options(kfr_dft_${ARCH} PRIVATE -Xclang -ffast-math)
endfunction()

add_dft_library(sse2)
add_dft_library(sse41)
add_dft_library(avx)
add_dft_library(avx2)
add_dft_library(avx512)