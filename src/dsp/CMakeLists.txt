cmake_minimum_required(VERSION 3.12)

add_kfr_library(NAME kfr_dsp MULTIARCH SOURCES ${KFR_DSP_SRC})

if (KFR_INSTALL_LIBRARIES)
    install(
        TARGETS ${kfr_dsp_TARGETS}
        EXPORT kfr_export
        ARCHIVE DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        LIBRARY DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        RUNTIME DESTINATION bin$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)
endif ()
