cmake_minimum_required(VERSION 3.28)

project(kfr-xcframework-app LANGUAGES CXX)

if (NOT KFR_XCFRAMEWORK)
    message(FATAL_ERROR "KFR_XCFRAMEWORK variable is not set")
endif ()

if (NOT EXISTS "${KFR_XCFRAMEWORK}")
    message(FATAL_ERROR "KFR_XCFRAMEWORK does not exist: ${KFR_XCFRAMEWORK}")
endif ()

add_executable(kfr-app kfr-app.cpp)
target_compile_features(kfr-app PRIVATE cxx_std_20)

# KFR_XCFRAMEWORK is a bare path to an .xcframework, not a CMake (imported)
# target. CMake/Xcode will pick the correct per-platform/arch slice from it
# automatically when linked this way, but because it isn't a target, it
# carries no INTERFACE_LINK_LIBRARIES / usage requirements — unlike linking
# against kfr_audio via find_package(KFR CONFIG), where AudioToolbox.framework
# would be pulled in transitively through target_link_libraries(). There is no
# CMake or Xcode mechanism to make a plain framework/library path carry
# transitive link requirements, so consumers of kfr.xcframework must add this
# dependency themselves whenever kfr_audio's functionality is used.
target_link_libraries(kfr-app PRIVATE "${KFR_XCFRAMEWORK}" "-framework AudioToolbox")
