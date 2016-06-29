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

add_compile_options(-fno-exceptions -fno-rtti -ftemplate-backtrace-limit=0)

link_libraries(stdc++ pthread)

include_directories(../include)

add_executable(basic_vector_test basic_vector_test.cpp ${KFR_SRC})
add_executable(dft_test dft_test.cpp ${KFR_SRC})
add_executable(empty_test empty_test.cpp ${KFR_SRC})
add_executable(complex_test complex_test.cpp ${KFR_SRC})

find_package(PythonInterp 2.7)

if (PYTHONINTERP_FOUND)
    enable_testing()

    add_test(NAME test_basic_vector
            COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/test_output.py
            ${PROJECT_BINARY_DIR}/tests/basic_vector_test
            ${PROJECT_SOURCE_DIR}/tests/basic_vector_test.cpp)
            
    add_test(NAME test_dft
            COMMAND ${PROJECT_BINARY_DIR}/tests/dft_test)
    add_test(NAME complex_test
            COMMAND ${PROJECT_BINARY_DIR}/tests/complex_test)
else ()
    message(WARNING "Install Python to run tests")
endif ()
