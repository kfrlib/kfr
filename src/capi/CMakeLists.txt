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

cmake_minimum_required(VERSION 3.12)

if (NOT WIN32)
    set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
    set(CMAKE_C_VISIBILITY_PRESET "hidden")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fdiagnostics-absolute-paths)
endif ()

add_library(kfr_capi SHARED capi.cpp)
target_link_libraries(kfr_capi PRIVATE kfr_dft kfr_dsp)

target_compile_definitions(kfr_capi PRIVATE KFR_BUILDING_DLL=1)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_property(
        TARGET kfr_capi
        APPEND
        PROPERTY LINK_LIBRARIES
                 -nodefaultlibs
                 -Wl,-Bdynamic
                 -lm
                 -lc
                 -Wl,-Bstatic
                 -lstdc++
                 -lgcc
                 -s)
endif ()

if (KFR_INSTALL_LIBRARIES)
    if (KFR_ENABLE_CAPI_BUILD)
        install(
            TARGETS kfr_capi
            EXPORT kfr_export
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
            )
    endif ()
endif ()
