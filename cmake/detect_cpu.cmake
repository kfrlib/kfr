function(detect_cpu KFR_SOURCE_DIR)

    if (NOT CPU_ARCH)
        set(CPU_ARCH detect)
    endif ()

    if (CPU_ARCH STREQUAL "detect" AND X86)
        message(STATUS "Detecting native cpu...")
        try_run(
            RUN_RESULT COMPILE_RESULT "${CMAKE_CURRENT_BINARY_DIR}/tmpdir"
            ${KFR_SOURCE_DIR}/cmake/detect_cpu.cpp
            CMAKE_FLAGS
                "-DINCLUDE_DIRECTORIES=${KFR_SOURCE_DIR}/include"
                -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
                -DCMAKE_CXX_EXTENSIONS=ON
            COMPILE_OUTPUT_VARIABLE COMPILE_OUT
            RUN_OUTPUT_VARIABLE RUN_OUT)
        if (COMPILE_RESULT AND RUN_RESULT EQUAL 0)
            message(STATUS DETECTED_CPU = ${RUN_OUT})
            set(CPU_ARCH
                ${RUN_OUT}
                CACHE STRING "Detected CPU" FORCE)
        else ()
            message(STATUS COMPILE_RESULT = ${COMPILE_RESULT})
            message(STATUS RUN_RESULT = ${RUN_RESULT})
            message(STATUS COMPILE_OUT = ${COMPILE_OUT})
            message(STATUS RUN_OUT = ${RUN_OUT})
        endif ()
    endif ()
    
endfunction()
