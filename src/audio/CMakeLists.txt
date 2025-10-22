cmake_minimum_required(VERSION 3.16)

add_kfr_library(
    NAME kfr_audio
    SOURCES ${KFR_AUDIO_SRC}
    PUBLIC_LIBRARIES kfr_io kfr_dsp)

target_include_directories(kfr_audio
                           PRIVATE ${PROJECT_SOURCE_DIR}/src/thirdparty)

# Cache Ogg path for cross-compiling
find_package(Ogg CONFIG CMAKE_FIND_ROOT_PATH_BOTH)

find_package(FLAC CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
if (TARGET FLAC::FLAC)
    get_target_property(_type FLAC::FLAC TYPE)
    if (_type STREQUAL "SHARED_LIBRARY")
        message(
            WARNING
                "FLAC::FLAC (${}) is a shared library, but kfr_audio requires the static FLAC library. Please install the static FLAC library and set CMAKE_PREFIX_PATH accordingly."
        )
    else ()
        message(STATUS "FLAC found, enabling FLAC support in kfr_audio")
        target_link_libraries(kfr_audio PUBLIC $<BUILD_INTERFACE:FLAC::FLAC>)
        target_compile_definitions(kfr_audio PUBLIC $<BUILD_INTERFACE:KFR_AUDIO_FLAC=1>)
        set(KFR_AUDIO_FLAC
            TRUE
            CACHE BOOL "FLAC support enabled in kfr_audio" FORCE)
    endif ()
else ()
    message(STATUS "FLAC not found, FLAC support in kfr_audio disabled")
    message(
        STATUS
            "To enable FLAC support, install FLAC using vcpkg, set CMAKE_PREFIX_PATH and re-run CMake"
    )
endif ()

if (TARGET Ogg::ogg)
    message(STATUS "Ogg found")
endif ()

find_path(
    ALAC_INCLUDE_DIRS "alac/ALACDecoder.h"
    PATH_SUFFIXES include
    CMAKE_FIND_ROOT_PATH_BOTH)
find_library(
    ALAC_LIBRARY
    NAMES libalac alac
    PATH_SUFFIXES lib
    CMAKE_FIND_ROOT_PATH_BOTH)

if (ALAC_INCLUDE_DIRS AND ALAC_LIBRARY)
    add_library(ALAC::ALAC UNKNOWN IMPORTED)
    set_target_properties(
        ALAC::ALAC
        PROPERTIES IMPORTED_LOCATION "${ALAC_LIBRARY}"
                   INTERFACE_INCLUDE_DIRECTORIES "${ALAC_INCLUDE_DIR}")
    message(STATUS "ALAC found, enabling ALAC support in kfr_audio")
    target_link_libraries(kfr_audio PUBLIC $<BUILD_INTERFACE:ALAC::ALAC>)
    target_compile_definitions(kfr_audio PUBLIC $<BUILD_INTERFACE:KFR_AUDIO_ALAC=1>)
    set(KFR_AUDIO_ALAC
        TRUE
        CACHE BOOL "ALAC support enabled in kfr_audio" FORCE)
else ()
    message(STATUS "ALAC not found, ALAC support in kfr_audio disabled")
    message(
        STATUS
            "To enable ALAC support, install ALAC using vcpkg, set CMAKE_PREFIX_PATH and re-run CMake"
    )
endif ()

if (WIN32)
    target_link_libraries(kfr_audio PUBLIC mf mfplat mfreadwrite mfuuid propsys)
endif ()

if (KFR_INSTALL_LIBRARIES)
    install(
        TARGETS kfr_audio
        EXPORT KFRTargets
        ARCHIVE DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        LIBRARY DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        RUNTIME DESTINATION bin$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)

    if (KFR_AUDIO_FLAC)
        if (TARGET FLAC::FLAC)
            install(
                FILES $<TARGET_FILE:FLAC::FLAC>
                DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)
        endif ()
        if (TARGET Ogg::ogg)
            install(
                FILES $<TARGET_FILE:Ogg::ogg>
                DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)
        endif ()
    endif ()
    if (KFR_AUDIO_ALAC)
        if (TARGET ALAC::ALAC)
            install(
                FILES $<TARGET_FILE:ALAC::ALAC>
                DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)
        endif ()
    endif ()
endif ()
