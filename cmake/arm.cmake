# Toolchain file for cross-compiling to ARM using Clang
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(EMULATOR qemu-arm)
set(QEMU_LD_PREFIX
    "/usr/arm-linux-gnueabihf/"
    CACHE INTERNAL "")
list(APPEND EMULATOR "-L")
list(APPEND EMULATOR ${QEMU_LD_PREFIX})

# Specify the Clang cross-compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_AR llvm-ar)
set(CMAKE_LINKER ld.lld)
set(CMAKE_NM llvm-nm)
set(CMAKE_OBJCOPY llvm-objcopy)
set(CMAKE_OBJDUMP llvm-objdump)
set(CMAKE_RANLIB llvm-ranlib)

# Set compiler flags for ARM
set(CMAKE_C_FLAGS
    "--target=arm-linux-gnueabihf -mcpu=cortex-a15 -mfpu=neon-vfpv4 -mfloat-abi=hard"
    CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS
    "--target=arm-linux-gnueabihf -mcpu=cortex-a15 -mfpu=neon-vfpv4 -mfloat-abi=hard"
    CACHE STRING "C++ flags")

# Set the target environment path
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)

# Adjust the default behavior of the FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Ensure CMake uses the cross-compiler for testing
set(CMAKE_CROSSCOMPILING_EMULATOR "")
