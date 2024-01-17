
function (link_as_whole TARGET TYPE LIBRARY)
    if (APPLE)
        target_link_options(${TARGET} ${TYPE} -Wl,-force_load
                            $<TARGET_FILE:${LIBRARY}>)
    elseif (WIN32)
        target_link_options(${TARGET} ${TYPE}
                            /WHOLEARCHIVE:$<TARGET_FILE:${LIBRARY}>)
    else ()
        target_link_options(${TARGET} ${TYPE} -Wl,--push-state,--whole-archive
                            $<TARGET_FILE:${LIBRARY}> -Wl,--pop-state)
    endif ()
endfunction ()
