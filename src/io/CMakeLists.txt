cmake_minimum_required(VERSION 3.12)

add_kfr_library(NAME kfr_io SOURCES ${KFR_IO_SRC})

if (KFR_INSTALL_LIBRARIES)
    install(
        TARGETS kfr_io
        EXPORT kfr_export
        ARCHIVE DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        LIBRARY DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        RUNTIME DESTINATION bin$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)
endif ()
