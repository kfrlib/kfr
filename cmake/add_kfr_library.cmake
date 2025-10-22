function (add_kfr_library)

    cmake_parse_arguments(
        LIB
        "MULTIARCH"
        "NAME"
        "SOURCES;LIBRARIES;PUBLIC_LIBRARIES;DEFINITIONS;PUBLIC_DEFINITIONS;INCLUDE_DIRECTORIES;OPTIONS"
        ${ARGN})

    set(${LIB_NAME}_LIBS)
    set(${LIB_NAME}_TARGETS)
    if (KFR_ENABLE_MULTIARCH AND LIB_MULTIARCH)
        add_library(${LIB_NAME} INTERFACE)
        list(GET KFR_ARCHS 0 BASE_ARCH)
        foreach (ARCH IN LISTS KFR_ARCHS)
            add_library(${LIB_NAME}_${ARCH} STATIC ${LIB_SOURCES})
            target_compile_definitions(${LIB_NAME}_${ARCH} PRIVATE KFR_MULTI=1)
            foreach (ENABLED_ARCH IN LISTS KFR_ARCHS)
                string(TOUPPER ${ENABLED_ARCH} ENABLED_ARCH_UPPER)
                target_compile_definitions(
                    ${LIB_NAME}_${ARCH}
                    PRIVATE KFR_MULTI_ENABLED_${ENABLED_ARCH_UPPER}=1)
            endforeach ()
            list(APPEND ${LIB_NAME}_LIBS ${LIB_NAME}_${ARCH})
            list(APPEND ${LIB_NAME}_TARGETS ${LIB_NAME}_${ARCH})
            target_link_libraries(${LIB_NAME} INTERFACE ${LIB_NAME}_${ARCH})
            target_set_arch(${LIB_NAME}_${ARCH} PRIVATE ${ARCH})
            if (NOT ARCH STREQUAL BASE_ARCH)
                target_compile_definitions(${LIB_NAME}_${ARCH} PRIVATE KFR_SKIP_IF_NON_X86=1)
            endif ()
        endforeach ()
        target_compile_definitions(${LIB_NAME}_${BASE_ARCH}
                                   PRIVATE KFR_BASE_ARCH=1)
        set_target_properties(${LIB_NAME}_${BASE_ARCH} PROPERTIES OUTPUT_NAME ${LIB_NAME})

        link_as_whole(${LIB_NAME} INTERFACE ${LIB_NAME}_${BASE_ARCH})

        list(APPEND ${LIB_NAME}_TARGETS ${LIB_NAME})
        target_compile_definitions(${LIB_NAME} INTERFACE $<BUILD_INTERFACE:${LIB_PUBLIC_DEFINITIONS}>)
    else ()
        add_library(${LIB_NAME} STATIC ${LIB_SOURCES})
        list(APPEND ${LIB_NAME}_LIBS ${LIB_NAME})
        list(APPEND ${LIB_NAME}_TARGETS ${LIB_NAME})
        target_set_arch(${LIB_NAME} PRIVATE ${KFR_ARCH})
    endif ()

    foreach (LIB IN LISTS ${LIB_NAME}_LIBS)
        target_link_libraries(${LIB} PUBLIC kfr)
        target_link_libraries(${LIB} PRIVATE ${LIB_LIBRARIES})
        target_link_libraries(${LIB} PUBLIC ${LIB_PUBLIC_LIBRARIES})
        target_compile_definitions(${LIB} PRIVATE ${LIB_DEFINITIONS})
        target_compile_definitions(${LIB} PUBLIC $<BUILD_INTERFACE:${LIB_PUBLIC_DEFINITIONS}>)
        target_compile_options(${LIB} PRIVATE ${LIB_OPTIONS})
        target_include_directories(${LIB} PRIVATE ${LIB_INCLUDE_DIRECTORIES})
    endforeach ()
    

    set(${LIB_NAME}_LIBS
        ${${LIB_NAME}_LIBS}
        PARENT_SCOPE)
    set(${LIB_NAME}_TARGETS
        ${${LIB_NAME}_TARGETS}
        PARENT_SCOPE)

endfunction ()
