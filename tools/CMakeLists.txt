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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)

add_executable(sample_rate_converter sample_rate_converter.cpp)
target_link_libraries(sample_rate_converter kfr kfr_io use_arch)

add_executable(ebu_test ebu_test.cpp)
target_link_libraries(ebu_test kfr kfr_io use_arch)
