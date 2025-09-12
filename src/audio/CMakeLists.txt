cmake_minimum_required(VERSION 3.12)

add_kfr_library(NAME kfr_audio SOURCES ${KFR_AUDIO_SRC})

find_package(FLAC CONFIG)
if (TARGET FLAC::FLAC)
    message(STATUS "FLAC found, enabling FLAC support in kfr_audio")
    target_link_libraries(kfr_audio PRIVATE FLAC::FLAC)
    target_compile_definitions(kfr_audio PUBLIC KFR_AUDIO_FLAC=1)
else ()
    message(WARNING "FLAC not found, FLAC support in kfr_audio disabled")
    message(
        STATUS
            "To enable FLAC support, install FLAC using vcpkg, set CMAKE_PREFIX_PATH and re-run CMake"
    )
endif ()

find_path(MINIMP3_INCLUDE_DIRS "minimp3/minimp3.h")
if (MINIMP3_INCLUDE_DIRS)
    message(STATUS "minimp3 found, enabling MP3 support in kfr_audio")
    target_include_directories(kfr_audio PRIVATE ${MINIMP3_INCLUDE_DIRS})
    target_compile_definitions(kfr_audio PUBLIC KFR_AUDIO_MP3=1)
else ()
    message(WARNING "minimp3 not found, MP3 support in kfr_audio disabled")
    message(
        STATUS
            "To enable MP3 support, install minimp3 using vcpkg, set CMAKE_PREFIX_PATH and re-run CMake"
    )
endif ()

find_path(ALAC_INCLUDE_DIRS "alac/ALACDecoder.h")
find_library(ALAC_LIBRARIES NAMES libalac alac)
if (ALAC_INCLUDE_DIRS AND ALAC_LIBRARIES)
    message(STATUS "ALAC found, enabling ALAC support in kfr_audio")
    target_include_directories(kfr_audio PRIVATE ${ALAC_INCLUDE_DIRS})
    target_link_libraries(kfr_audio PRIVATE ${ALAC_LIBRARIES})
    target_compile_definitions(kfr_audio PUBLIC KFR_AUDIO_ALAC=1)
else ()
    message(WARNING "ALAC not found, ALAC support in kfr_audio disabled")
    message(
        STATUS
            "To enable ALAC support, install ALAC using vcpkg, set CMAKE_PREFIX_PATH and re-run CMake"
    )
endif ()

if (WIN32)
    target_link_libraries(kfr_audio PUBLIC mf mfplat mfreadwrite mfuuid propsys)
endif()

if (KFR_INSTALL_LIBRARIES)
    install(
        TARGETS kfr_audio
        EXPORT kfr_export
        ARCHIVE DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        LIBRARY DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        RUNTIME DESTINATION bin$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)
endif ()
