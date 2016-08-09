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

if (NOT MSVC)
    add_compile_options(-fno-exceptions -fno-rtti)
    if (NOT ARCH_FLAGS)
        add_compile_options(-march=native)
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
    endif ()
    link_libraries(${STD_LIB} pthread)
else ()
    add_compile_options(/arch:AVX)
endif ()

include_directories(../include)

add_executable(biquads biquads.cpp ${KFR_SRC})
add_executable(window window.cpp ${KFR_SRC})
add_executable(fir fir.cpp ${KFR_SRC})
add_executable(sample_rate_conversion sample_rate_conversion.cpp ${KFR_SRC})
add_executable(dft dft.cpp ${KFR_SRC} ${DFT_SRC})
