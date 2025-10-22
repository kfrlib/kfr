cmake_minimum_required(VERSION 3.16)

include(FetchContent)

set(KFR_DSP_DEFS)
set(KFR_DSP_LIBS)

if (KFR_USE_BOOST OR KFR_USE_BOOST_MATH)

    if (KFR_USE_BOOST)
        find_package(
            Boost
            COMPONENTS
            REQUIRED math CMAKE_FIND_ROOT_PATH_BOTH)

        if (TARGET Boost::math)
            message(STATUS "Boost found, enabling elliptic filters in kfr_dsp")
        endif ()
    else () # KFR_USE_BOOST_MATH
        message(
            STATUS
                "Enabling elliptic filters in kfr_dsp using Boost.Main (standalone)"
        )
        set(BOOST_MATH_STANDALONE
            TRUE
            CACHE BOOL "Standalone Boost.Math" FORCE)

        FetchContent_Declare(
            boost_math
            GIT_REPOSITORY https://github.com/boostorg/math.git
            GIT_TAG boost-1.89.0 # Specific stable release
        )
        FetchContent_MakeAvailable(boost_math)
    endif ()
    list(APPEND KFR_DSP_DEFS KFR_HAVE_ELLIPTIC=1)
    list(APPEND KFR_DSP_LIBS $<BUILD_INTERFACE:Boost::math>)

endif ()

add_kfr_library(
    NAME kfr_dsp
    MULTIARCH
    SOURCES ${KFR_DSP_SRC}
    LIBRARIES ${KFR_DSP_LIBS}
    PUBLIC_DEFINITIONS ${KFR_DSP_DEFS})

if (KFR_INSTALL_LIBRARIES)
    install(
        TARGETS ${kfr_dsp_TARGETS}
        EXPORT KFRTargets
        ARCHIVE DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        LIBRARY DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        RUNTIME DESTINATION bin$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)
endif ()
