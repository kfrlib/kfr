
function (link_as_whole TARGET TYPE LIBRARY)
    if (APPLE)
        target_link_options(${TARGET} ${TYPE} "LINKER:-force_load,$<TARGET_FILE:${LIBRARY}>")
    elseif (MSVC)
        target_link_options(${TARGET} ${TYPE}
                            /WHOLEARCHIVE:$<TARGET_FILE:${LIBRARY}>)
    else ()
        target_link_options(${TARGET} ${TYPE} "LINKER:--push-state,--whole-archive,$<TARGET_FILE:${LIBRARY}>,--pop-state")
    endif ()
endfunction ()
