set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_VERSION 1)
set (UNIX True)
set (ARM True)
set (CMAKE_SYSTEM_PROCESSOR arm)

include (CMakeForceCompiler)
CMAKE_FORCE_CXX_COMPILER (/usr/bin/clang++ Clang)
CMAKE_FORCE_C_COMPILER (/usr/bin/clang Clang)
set (CMAKE_CXX_COMPILER_WORKS TRUE)
set (CMAKE_C_COMPILER_WORKS TRUE)

set (ARM_ROOT "/usr/arm-linux-gnueabihf/include")
set (GCC_VER 5.4.0)
set (SYS_PATHS "-isystem ${ARM_ROOT}/c++/${GCC_VER} -isystem ${ARM_ROOT}/c++/${GCC_VER}/backward -isystem ${ARM_ROOT}/c++/${GCC_VER}/arm-linux-gnueabihf -isystem ${ARM_ROOT}")

set (ARM_COMMON_FLAGS "-target arm-linux-gnueabihf -mcpu=cortex-a15 -mfpu=neon-vfpv4 -mfloat-abi=hard -static")

set (CMAKE_CXX_FLAGS "${SYS_PATHS} ${ARM_COMMON_FLAGS}")
set (CMAKE_C_FLAGS " ${SYS_PATHS} ${ARM_COMMON_FLAGS}")

set (CMAKE_CXX_LINK_FLAGS " ${ARM_COMMON_FLAGS} ${CMAKE_CXX_LINK_FLAGS}")
set (CMAKE_C_LINK_FLAGS " ${ARM_COMMON_FLAGS} ${CMAKE_C_LINK_FLAGS}")

message(STATUS "${ARM_COMMON_FLAGS}")

set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
