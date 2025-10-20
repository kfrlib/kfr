cmake_minimum_required(VERSION 3.12)

set(KFR_DSP_DEFS)

if (KFR_USE_BOOST)

    set(KFR_DSP_LIBS)
    find_package(Boost COMPONENTS math CMAKE_FIND_ROOT_PATH_BOTH)

    if (TARGET Boost::math)
        list(APPEND KFR_DSP_LIBS Boost::math)
        list(APPEND KFR_DSP_DEFS KFR_HAVE_ELLIPTIC=1)
        message(STATUS "Boost found, enabling elliptic filters in kfr_dsp")
    endif ()

    add_kfr_library(
        NAME kfr_dsp
        MULTIARCH
        SOURCES ${KFR_DSP_SRC}
        LIBRARIES ${KFR_DSP_LIBS}
        PUBLIC_DEFINITIONS ${KFR_DSP_DEFS})

else ()

    set(KFR_DSP_INCDIRS)

    find_path(
        BOOST_MATH_HEADER boost/math/special_functions/ellint_1.hpp
        PATH_SUFFIXES include
        CMAKE_FIND_ROOT_PATH_BOTH)

    if (BOOST_MATH_HEADER)
        list(APPEND KFR_DSP_INCDIRS ${BOOST_MATH_HEADER})
        list(APPEND KFR_DSP_DEFS KFR_HAVE_ELLIPTIC=1)
        message(STATUS "Boost.Math found, enabling elliptic filters in kfr_dsp")
    endif ()

    add_kfr_library(
        NAME kfr_dsp
        MULTIARCH
        SOURCES ${KFR_DSP_SRC}
        PUBLIC_DEFINITIONS ${KFR_DSP_DEFS}
        INCLUDE_DIRECTORIES ${KFR_DSP_INCDIRS})

endif ()

if (KFR_INSTALL_LIBRARIES)
    install(
        TARGETS ${kfr_dsp_TARGETS}
        EXPORT KFRTargets
        ARCHIVE DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        LIBRARY DESTINATION lib$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>
        RUNTIME DESTINATION bin$<$<CONFIG:Debug>:${KFR_DEBUG_INSTALL_SUFFIX}>)
endif ()
