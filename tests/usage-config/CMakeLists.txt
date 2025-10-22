cmake_minimum_required(VERSION 3.12)

project(kfr-app)

find_package(KFR CONFIG REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)

add_executable(kfr-app kfr-app.cpp)

target_link_libraries(kfr-app kfr kfr_dsp kfr_dft kfr_io kfr_audio)

enable_testing()

if (NOT CMAKE_CROSSCOMPILING)
    add_test(NAME kfr-app-test COMMAND kfr-app)
endif ()
