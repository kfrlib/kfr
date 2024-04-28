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

if (KFR_WITH_CLANG)
    set(CLANG_VERSION 17.0.6)
    if (WIN32)
        set(CLANG_URL
            https://github.com/llvm/llvm-project/releases/download/llvmorg-${CLANG_VERSION}/LLVM-${CLANG_VERSION}-win64.exe
        )
        set(CLANG_ARCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/clang-prebuilt.exe")
        set(CLANG_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/clang")

        set(SEVENZIP_URL https://www.7-zip.org/a/7z2301-x64.msi)
        set(SEVENZIP_ARCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/7z2301-x64.msi")
        set(SEVENZIP_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/7z")
        message(STATUS "Downloading 7zip from ${SEVENZIP_URL}")
        file(
            DOWNLOAD ${SEVENZIP_URL} ${SEVENZIP_ARCH_FILE}
            EXPECTED_HASH
                SHA256=0BA639B6DACDF573D847C911BD147C6384381A54DAC082B1E8C77BC73D58958B
            SHOW_PROGRESS)

        cmake_path(NATIVE_PATH SEVENZIP_ARCH_FILE NORMALIZE
                   SEVENZIP_ARCH_FILE_W)
        cmake_path(NATIVE_PATH SEVENZIP_DEST_DIR NORMALIZE SEVENZIP_DEST_DIR_W)
        execute_process(COMMAND msiexec /a "${SEVENZIP_ARCH_FILE_W}"
                                "TARGETDIR=${SEVENZIP_DEST_DIR_W}" /qn)

        message(STATUS "Downloading Clang ${CLANG_VERSION} from ${CLANG_URL}")
        file(
            DOWNLOAD ${CLANG_URL} ${CLANG_ARCH_FILE}
            EXPECTED_HASH
                SHA256=89dc351af8e8fa1cafc6b48411e608aa9457c930a10f984aad5d21ab020165b2
            SHOW_PROGRESS)

        message(STATUS "Unpacking Clang ${CLANG_VERSION}")
        execute_process(
            COMMAND ${SEVENZIP_DEST_DIR}/Files/7-Zip/7z.exe x -bb0 -bd -aos
                    -o${CLANG_DEST_DIR} ${CLANG_ARCH_FILE} OUTPUT_QUIET)

        set(CMAKE_CXX_COMPILER
            ${CLANG_DEST_DIR}/bin/clang-cl.exe
            CACHE STRING "Clang" FORCE)
        set(CMAKE_C_COMPILER
            ${CLANG_DEST_DIR}/bin/clang-cl.exe
            CACHE STRING "Clang" FORCE)
    endif ()
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        set(CLANG_URL
            https://github.com/llvm/llvm-project/releases/download/llvmorg-${CLANG_VERSION}/clang+llvm-${CLANG_VERSION}-x86_64-linux-gnu-ubuntu-22.04.tar.xz
        )
        set(CLANG_ARCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/clang-prebuilt.tar.xz")
        set(CLANG_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/clang")
        message(STATUS "Downloading Clang ${CLANG_VERSION} from ${CLANG_URL}")
        file(
            DOWNLOAD ${CLANG_URL} ${CLANG_ARCH_FILE}
            EXPECTED_HASH
                SHA256=884ee67d647d77e58740c1e645649e29ae9e8a6fe87c1376be0f3a30f3cc9ab3
            SHOW_PROGRESS)
        message(STATUS "Unpacking Clang ${CLANG_VERSION}")
        file(ARCHIVE_EXTRACT INPUT ${CLANG_ARCH_FILE} DESTINATION
             ${CLANG_DEST_DIR})
        set(CMAKE_CXX_COMPILER
            ${CLANG_DEST_DIR}/clang+llvm-${CLANG_VERSION}-x86_64-linux-gnu-ubuntu-22.04/bin/clang++
            CACHE STRING "Clang" FORCE)
        set(CMAKE_C_COMPILER
            ${CLANG_DEST_DIR}/clang+llvm-${CLANG_VERSION}-x86_64-linux-gnu-ubuntu-22.04/bin/clang
            CACHE STRING "Clang" FORCE)
    endif ()
    message(STATUS "Clang ${CLANG_VERSION} is set up successfully")
endif ()

file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/include/kfr/kfr.h KFR_VERSION
     REGEX "#define KFR_VERSION_(MINOR|MAJOR|PATCH)")
string(REGEX MATCHALL "[0-9]+" KFR_VERSION_MATCH ${KFR_VERSION})
string(REPLACE ";" "." KFR_VERSION "${KFR_VERSION_MATCH}")

project(
    kfr
    VERSION ${KFR_VERSION}
    LANGUAGES CXX)
    
if (DEBUG_CMAKE)
    get_cmake_property(VARS VARIABLES)
    list (SORT VARS)
    foreach (VAR ${VARS})
        message(STATUS "${VAR}=${${VAR}}")
    endforeach()
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib)

if (CPU_ARCH)
    if (NOT KFR_ARCH)
        message(
            WARNING
                "CPU_ARCH is deprecated. Use KFR_ARCH instead. KFR will use machine native architecture by default"
        )
    else ()
        message(
            WARNING
                "Both KFR_ARCH and CPU_ARCH are defined. KFR will use architecture set by KFR_ARCH (${KFR_ARCH})"
        )
    endif ()
endif ()

if (EMSCRIPTEN)
    set(KFR_ARCH "sse41" CACHE STRING "")
    set(KFR_ENABLE_MULTIARCH OFF CACHE BOOL "")
endif ()

set(X86 FALSE)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set(X86 TRUE)
endif ()
if (CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
    set(X86 TRUE)
endif ()
if (CMAKE_OSX_ARCHITECTURES MATCHES "^[Aa][Rr][Mm]64$")
    set(X86 FALSE)
endif ()

if (CMAKE_CXX_COMPILER_TARGET MATCHES "^arm")
    set(X86 FALSE)
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BITNESS64 TRUE)
else ()
    set(BITNESS64 FALSE)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL
                                              "AppleClang")
    set(CLANG 1)
else ()
    set(CLANG 0)
endif ()

include(cmake/target_set_arch.cmake)
include(cmake/link_as_whole.cmake)
include(cmake/add_kfr_library.cmake)

# Include autogenerated list of source files
include(sources.cmake)
include(CMakeDependentOption)

option(KFR_ENABLE_MULTIARCH
       "Multiple architectures will be built with runtime dispatch" ON)

option(ENABLE_TESTS "Enable KFR tests" OFF)
cmake_dependent_option(ENABLE_EXAMPLES "Enable KFR examples" ON "ENABLE_TESTS"
                       OFF)
if (CLANG)
    option(KFR_ENABLE_DFT "Enable DFT and related algorithms." ON)
else ()
    option(KFR_ENABLE_DFT "Enable DFT and related algorithms." OFF)
endif ()
option(KFR_ENABLE_ASMTEST "Enable writing disassembly" OFF)
option(KFR_REGENERATE_TESTS "Regenerate auto tests" OFF)
option(KFR_DISABLE_CLANG_EXTENSIONS "Disable Clang vector extensions" OFF)
option(KFR_EXTENDED_TESTS "Extended tests (up to hour)" OFF)
option(KFR_SKIP_TESTS "Build tests but don't run tests" OFF)
option(KFR_ENABLE_CAPI_BUILD "Enable KFR C API building" OFF)
option(KFR_INSTALL_HEADERS "Include headers in installation" ON)
option(KFR_INSTALL_LIBRARIES "Include libraries in installation" ON)
mark_as_advanced(KFR_ENABLE_ASMTEST)
mark_as_advanced(KFR_REGENERATE_TESTS)
mark_as_advanced(KFR_DISABLE_CLANG_EXTENSIONS)

if (KFR_ENABLE_CAPI_BUILD AND NOT KFR_ENABLE_DFT)
    message(
        FATAL_ERROR
            "KFR_ENABLE_CAPI_BUILD requires KFR_ENABLE_DFT to be enabled")
endif ()

if (NOT KFR_ARCH)
    set(KFR_ARCH target)
endif ()

if (KFR_ARCH STREQUAL "detect")
    set(KFR_ARCH host)
endif ()

set(DETECT_NAMES host target)
if (X86)
    set(ALLOWED_ARCHS
        generic
        sse
        sse2
        sse3
        ssse3
        sse41
        sse42
        avx
        avx2
        avx512)
else ()
    set(ALLOWED_ARCHS generic neon neon64)
endif ()

if (NOT X86)
    if (KFR_ARCH IN_LIST DETECT_NAMES)
        if (BITNESS64)
            set(KFR_ARCH neon64)
        else ()
            set(KFR_ARCH neon)
        endif ()
    endif ()
endif ()

if (KFR_ARCH IN_LIST DETECT_NAMES)
    message(STATUS "Detecting ${KFR_ARCH} architecture")
    try_run(
        RUN_RESULT COMPILE_RESULT "${CMAKE_CURRENT_BINARY_DIR}/tmpdir"
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/detect_cpu.cpp
        CMAKE_FLAGS
            "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}/include"
            -DCMAKE_CXX_STANDARD=17
            -DCMAKE_CXX_STANDARD_REQUIRED=ON
            "-DCOMPILE_DEFINITIONS=-DCPU_${KFR_ARCH}=1"
            -DCMAKE_CXX_EXTENSIONS=ON
        COMPILE_OUTPUT_VARIABLE COMPILE_OUT
        RUN_OUTPUT_VARIABLE RUN_OUT)
    if (COMPILE_RESULT AND RUN_RESULT EQUAL 0)
        message(
            STATUS
                "Default CPU architecture for KFR is ${RUN_OUT} (set KFR_ARCH to override)"
        )
        set(KFR_ARCH
            ${RUN_OUT}
            CACHE STRING "Detected CPU" FORCE)
    else ()
        message(STATUS COMPILE_RESULT = ${COMPILE_RESULT})
        message(STATUS RUN_RESULT = ${RUN_RESULT})
        message(STATUS COMPILE_OUT = ${COMPILE_OUT})
        message(FATAL_ERROR RUN_OUT = ${RUN_OUT})
    endif ()
else ()
    message(
        STATUS
            "Default CPU architecture for KFR is ${KFR_ARCH} (set by KFR_ARCH)")
endif ()

if (NOT KFR_ARCH IN_LIST ALLOWED_ARCHS)
    message(FATAL_ERROR "Incorrect architecture set by KFR_ARCH: ${KFR_ARCH}")
endif ()

if (NOT KFR_ARCHS)
    if (X86)
        if (APPLE)
            set(KFR_ARCHS sse41 avx avx2 avx512)
        else ()
            set(KFR_ARCHS sse2 sse41 avx avx2 avx512)
        endif ()
    else ()
        set(KFR_ARCHS ${KFR_ARCH})
    endif ()
endif ()

if (KFR_ENABLE_MULTIARCH)
    message(
        STATUS
            "Runtime dispatch is enabled for architectures: ${KFR_ARCHS} (set KFR_ARCHS to change, set KFR_ENABLE_MULTIARCH=OFF to disable)"
    )
else ()
    message(
        STATUS
            "Runtime dispatch is disabled. Set KFR_ENABLE_MULTIARCH=ON to enable"
    )
endif ()

string(REPLACE ";" ", " KFR_ARCHS_COMMA "${KFR_ARCHS}")

if (KFR_ENABLE_MULTIARCH)
    add_compile_definitions(KFR_ENABLED_ARCHS="${KFR_ARCHS_COMMA}")
endif ()

add_library(use_arch INTERFACE)
target_set_arch(use_arch INTERFACE ${KFR_ARCH})

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
endif ()

if (IOS)
    set(STD_LIB)
else ()
    set(STD_LIB stdc++)
endif ()

if (ANDROID)
    set(PTHREAD_LIB)
else ()
    set(PTHREAD_LIB pthread)
endif ()

# KFR library
add_library(kfr INTERFACE)
# target_sources(kfr INTERFACE ${KFR_SRC})
target_include_directories(
    kfr INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                  $<INSTALL_INTERFACE:include>)
target_compile_options(kfr INTERFACE "$<$<CONFIG:DEBUG>:-DKFR_DEBUG>")
if (APPLE)
    target_compile_options(kfr INTERFACE -faligned-allocation)
endif ()

if (EMSCRIPTEN)
    target_compile_options(kfr INTERFACE -msimd128)
endif ()

if (MSVC)
    target_compile_options(kfr INTERFACE -bigobj)
else ()
    target_link_libraries(kfr INTERFACE ${STD_LIB} ${PTHREAD_LIB} m)
endif ()
if (KFR_DISABLE_CLANG_EXTENSIONS)
    target_compile_definitions(kfr INTERFACE -DCMT_DISABLE_CLANG_EXT)
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-ignored-qualifiers -Wno-psabi)
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-c++1z-extensions -Wno-psabi
                        -Wno-unknown-warning-option)
endif ()
if (MSVC)
    add_compile_options(/wd4141)
    target_compile_options(kfr INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/Zc:lambda>)
endif ()

if (NOT KFR_ENABLE_DFT)
    target_compile_definitions(kfr INTERFACE -DKFR_NO_DFT)
endif ()
if (KFR_EXTENDED_TESTS)
    target_compile_definitions(kfr INTERFACE -DKFR_EXTENDED_TESTS)
endif ()

set(KFR_DEBUG_INSTALL_SUFFIX "/debug" CACHE STRING "")

add_subdirectory(src/dsp)
add_subdirectory(src/io)
if (KFR_ENABLE_DFT)
    add_subdirectory(src/dft)
endif ()
if (KFR_ENABLE_CAPI_BUILD)
    add_subdirectory(src/capi)
endif ()

if (ENABLE_EXAMPLES)
    add_subdirectory(examples)
    add_subdirectory(tools)
endif ()
if (ENABLE_TESTS)
    add_subdirectory(tests)
endif ()

set(kfr_defines)

function (append_defines_from target)
    get_target_property(compile_defs ${target} INTERFACE_COMPILE_DEFINITIONS)
    if (compile_defs)
        list(APPEND kfr_defines "${compile_defs}")
    endif ()
    set(kfr_defines
        ${kfr_defines}
        PARENT_SCOPE)
endfunction ()

append_defines_from(kfr)
if (KFR_ENABLE_DFT)
    append_defines_from(kfr_dft)
endif ()
append_defines_from(kfr_io)

string(REPLACE "=" " " kfr_defines "${kfr_defines}")
string(REPLACE ";" "\n#define " kfr_defines "${kfr_defines}")
if (kfr_defines)
    set(kfr_defines "#define ${kfr_defines}\n")
endif ()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/kfr_config.h "${kfr_defines}")

if (KFR_INSTALL_HEADERS)
    install(DIRECTORY include/kfr DESTINATION include)

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/kfr_config.h
        DESTINATION include/kfr
        RENAME config.h)
endif ()

# uninstall target
if (NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake" IMMEDIATE
        @ONLY)

    add_custom_target(
        uninstall
        COMMAND ${CMAKE_COMMAND} -P
                ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(KFR_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/kfr"
    CACHE STRING "Path to KFR CMake files")

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/KFRConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/kfr"
    NO_SET_AND_CHECK_MACRO NO_CHECK_REQUIRED_COMPONENTS_MACRO)

install(TARGETS kfr EXPORT kfr_export)

install(
    EXPORT kfr_export
    DESTINATION "${KFR_INSTALL_CMAKEDIR}"
    CONFIGURATIONS Debug Release
    FILE KFRConfig.cmake)

write_basic_package_version_file(KFRConfigVersion.cmake
    VERSION ${KFR_VERSION} 
    COMPATIBILITY SameMinorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/KFRConfigVersion.cmake"
    DESTINATION
    "${KFR_INSTALL_CMAKEDIR}" )
