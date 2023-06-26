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

# Binary output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/svg)

add_executable(biquads biquads.cpp)
target_link_libraries(biquads kfr use_arch)

add_executable(iir iir.cpp)
target_link_libraries(iir kfr use_arch)

add_executable(window window.cpp)
target_link_libraries(window kfr use_arch)

add_executable(fir fir.cpp)
target_link_libraries(fir kfr use_arch)

if (KFR_ENABLE_DFT)
    target_link_libraries(fir kfr_dft use_arch)
    target_compile_definitions(fir PRIVATE -DHAVE_DFT)
endif ()

add_executable(sample_rate_conversion sample_rate_conversion.cpp)
target_link_libraries(sample_rate_conversion kfr kfr_io use_arch)

if (KFR_ENABLE_DFT)
    add_executable(dft dft.cpp)
    target_link_libraries(dft kfr kfr_dft use_arch)
    add_executable(ccv ccv.cpp)
    target_link_libraries(ccv kfr kfr_dft use_arch)
endif ()
