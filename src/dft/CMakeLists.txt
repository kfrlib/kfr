cmake_minimum_required(VERSION 3.12)

add_kfr_library(NAME kfr_dft MULTIARCH SOURCES ${KFR_DFT_SRC})

function (dft_compile_options LIB)
    if (MSVC AND CLANG)
        target_compile_options(${LIB} PRIVATE SHELL:-Xclang -ffp-contract=fast
                                              -Xclang -O3)
    else ()
        target_compile_options(${LIB} PRIVATE -ffp-contract=fast -O3)
    endif ()
endfunction ()

foreach (LIB IN LISTS kfr_dft_LIBS)
    dft_compile_options(${LIB})
endforeach ()

if (KFR_INSTALL_LIBRARIES)
    install(
        TARGETS ${kfr_dft_TARGETS}
        EXPORT kfr_export
        ARCHIVE DESTINATION lib$<$<CONFIG:Debug>:/debug>
        LIBRARY DESTINATION lib$<$<CONFIG:Debug>:/debug>
        RUNTIME DESTINATION bin$<$<CONFIG:Debug>:/debug>)
endif ()
