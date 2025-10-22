cmake_minimum_required(VERSION 3.16)

project(kfr-app)

if (NOT KFR_DIR)
    message(
        FATAL_ERROR
            "KFR_DIR variable is not set. Please set it to the path where KFR binaries are located."
    )
endif ()

add_executable(kfr-app kfr-app.cpp)

target_compile_features(kfr-app PRIVATE cxx_std_20)

target_include_directories(kfr-app PRIVATE ${KFR_DIR}/include)

set(kfr_dsp_LIBRARY "${KFR_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}kfr_dsp${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(kfr_dft_LIBRARY "${KFR_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}kfr_dft${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(kfr_io_LIBRARY "${KFR_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}kfr_io${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(kfr_audio_LIBRARY "${KFR_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}kfr_audio${CMAKE_STATIC_LIBRARY_SUFFIX}")

set(kfr_dsp_DBG_LIBRARY "${KFR_DIR}/lib/debug/${CMAKE_STATIC_LIBRARY_PREFIX}kfr_dsp${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(kfr_dft_DBG_LIBRARY "${KFR_DIR}/lib/debug/${CMAKE_STATIC_LIBRARY_PREFIX}kfr_dft${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(kfr_io_DBG_LIBRARY "${KFR_DIR}/lib/debug/${CMAKE_STATIC_LIBRARY_PREFIX}kfr_io${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(kfr_audio_DBG_LIBRARY "${KFR_DIR}/lib/debug/${CMAKE_STATIC_LIBRARY_PREFIX}kfr_audio${CMAKE_STATIC_LIBRARY_SUFFIX}")

if (EXISTS ${kfr_dsp_DBG_LIBRARY}
    AND EXISTS ${kfr_dft_DBG_LIBRARY}
    AND EXISTS ${kfr_io_DBG_LIBRARY}
    AND EXISTS ${kfr_audio_DBG_LIBRARY})
    message(STATUS "Debug libraries found.")

    target_link_libraries(
        kfr-app
        debug
        ${kfr_dsp_DBG_LIBRARY}
        debug
        ${kfr_dft_DBG_LIBRARY}
        debug
        ${kfr_io_DBG_LIBRARY}
        debug
        ${kfr_audio_DBG_LIBRARY})
else ()
    message(
        WARNING
            "Debug libraries not found. Make sure debug versions of KFR libraries are installed."
    )
endif ()

if (EXISTS ${kfr_dsp_LIBRARY}
    AND EXISTS ${kfr_dft_LIBRARY}
    AND EXISTS ${kfr_io_LIBRARY}
    AND EXISTS ${kfr_audio_LIBRARY})
    message(STATUS "Release libraries found.")

    target_link_libraries(
        kfr-app
        optimized
        ${kfr_dsp_LIBRARY}
        optimized
        ${kfr_dft_LIBRARY}
        optimized
        ${kfr_io_LIBRARY}
        optimized
        ${kfr_audio_LIBRARY})
else ()
    message(
        WARNING
            "Release libraries not found. Make sure KFR libraries are installed."
    )
endif ()

enable_testing()

if (NOT CMAKE_CROSSCOMPILING)
    add_test(NAME kfr-app-test COMMAND kfr-app)
endif ()
