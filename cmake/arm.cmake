# For internal use only

set(TGT_TRIPLET arm-linux-gnueabihf)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(UNIX True)
set(ARM True)

set(CMAKE_SYSTEM_PROCESSOR arm)
set(EMULATOR qemu-arm)
set(QEMU_LD_PREFIX "/usr/${TGT_TRIPLET}/" CACHE INTERNAL "")
list(APPEND EMULATOR "-L")
list(APPEND EMULATOR ${QEMU_LD_PREFIX})

if (NOT CLANG_SUFFIX)
    set(CLANG_SUFFIX "")
endif ()

set(CMAKE_CXX_COMPILER
    /usr/bin/clang++${CLANG_SUFFIX}
    CACHE INTERNAL "Clang")
set(CMAKE_C_COMPILER
    /usr/bin/clang${CLANG_SUFFIX}
    CACHE INTERNAL "Clang")
set(CMAKE_CXX_COMPILER_TARGET
    ${TGT_TRIPLET}
    CACHE INTERNAL "Clang target")
set(CMAKE_C_COMPILER_TARGET
    ${TGT_TRIPLET}
    CACHE INTERNAL "Clang target")

set(ARM_ROOT "/usr/${TGT_TRIPLET}/include")
if (NOT GCC_VER)
    set(GCC_VER 7.5.0)
endif ()
set(SYS_PATHS
    "-isystem ${ARM_ROOT}/c++/${GCC_VER} -isystem ${ARM_ROOT}/c++/${GCC_VER}/backward -isystem ${ARM_ROOT}/c++/${GCC_VER}/${TGT_TRIPLET} -isystem ${ARM_ROOT}"
)

set(ARM_COMMON_FLAGS
    "-target ${TGT_TRIPLET} -mcpu=cortex-a15 -mfpu=neon-vfpv4 -mfloat-abi=hard")

set(CMAKE_CXX_FLAGS
    "${SYS_PATHS} ${ARM_COMMON_FLAGS}"
    CACHE STRING "")
set(CMAKE_C_FLAGS
    " ${SYS_PATHS} ${ARM_COMMON_FLAGS}"
    CACHE STRING "")

set(CMAKE_CXX_LINK_FLAGS " ${ARM_COMMON_FLAGS} ${CMAKE_CXX_LINK_FLAGS}")
set(CMAKE_C_LINK_FLAGS " ${ARM_COMMON_FLAGS} ${CMAKE_C_LINK_FLAGS}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
