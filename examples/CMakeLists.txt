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

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/svg)

include_directories(../include)

add_executable(biquads biquads.cpp)
target_link_libraries(biquads kfr)

add_executable(window window.cpp)
target_link_libraries(window kfr)

add_executable(fir fir.cpp)
target_link_libraries(fir kfr kfr_dft)

add_executable(sample_rate_conversion sample_rate_conversion.cpp)
target_link_libraries(sample_rate_conversion kfr kfr_io)

add_executable(sample_rate_converter sample_rate_converter.cpp)
target_link_libraries(sample_rate_converter kfr kfr_io)

add_executable(dft dft.cpp)
target_link_libraries(dft kfr kfr_dft)
