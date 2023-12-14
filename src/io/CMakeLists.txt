cmake_minimum_required(VERSION 3.12)

add_kfr_library(NAME kfr_io SOURCES ${KFR_IO_SRC})

if (KFR_INSTALL_LIBRARIES)
    install(
        TARGETS kfr_io
        EXPORT kfr_export
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
endif ()
