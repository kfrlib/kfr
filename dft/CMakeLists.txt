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
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif ()

if(APPLE)
    add_compile_options(-mmacosx-version-min=10.9)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib)

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

add_library(kfr_multidft INTERFACE)
target_link_libraries(kfr_multidft INTERFACE kfr)
if (NOT WIN32)
    add_library(kfr_multidft_pic INTERFACE)
    target_link_libraries(kfr_multidft_pic INTERFACE kfr)
endif ()

function(add_dft_library ARCH)
    add_library(kfr_dft_${LIBABI}_${ARCH} STATIC ${KFR_DFT_SRC})
    target_link_libraries(kfr_dft_${LIBABI}_${ARCH} kfr)
    target_set_arch(kfr_dft_${LIBABI}_${ARCH} PRIVATE ${ARCH})
    target_compile_options(kfr_dft_${LIBABI}_${ARCH} PRIVATE -Xclang -ffast-math)
    add_dependencies(kfr_multiarch_dft kfr_dft_${LIBABI}_${ARCH})
    target_link_libraries(kfr_multidft INTERFACE kfr_dft_${LIBABI}_${ARCH})

    if (NOT WIN32)
        add_library(kfr_dft_${LIBABI}_${ARCH}_pic STATIC ${KFR_DFT_SRC})
        set_property(TARGET kfr_dft_${LIBABI}_${ARCH}_pic PROPERTY POSITION_INDEPENDENT_CODE 1)
        target_link_libraries(kfr_dft_${LIBABI}_${ARCH}_pic kfr)
        target_set_arch(kfr_dft_${LIBABI}_${ARCH}_pic PRIVATE ${ARCH})
        target_compile_options(kfr_dft_${LIBABI}_${ARCH}_pic PRIVATE -Xclang -ffast-math)

        target_link_libraries(kfr_multidft_pic INTERFACE kfr_dft_${LIBABI}_${ARCH}_pic)
    endif()
endfunction()

add_dft_library(sse2)
add_dft_library(sse41)
add_dft_library(avx)
add_dft_library(avx2)
add_dft_library(avx512)

add_library(kfrdft SHARED ${PROJECT_SOURCE_DIR}/include/kfr/dft/impl/dft-src.cpp)
target_compile_definitions(kfrdft PRIVATE -DKFR_DFT_MULTI=1 -DKFR_BUILDING_DLL=1)

if (WIN32)
    target_link_libraries(kfrdft PRIVATE kfr kfr_multidft)
else ()
    target_link_libraries(kfrdft PRIVATE kfr kfr_multidft_pic)

    if (APPLE)
    else ()
        set_property(TARGET kfrdft APPEND PROPERTY LINK_LIBRARIES -nodefaultlibs -Wl,-Bdynamic -lm -lc -Wl,-Bstatic -lstdc++ -lgcc -s)
    endif ()
endif()
