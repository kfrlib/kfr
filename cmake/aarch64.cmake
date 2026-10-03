# Toolchain file for cross-compiling to AARch64 using Clang
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(EMULATOR qemu-aarch64)
set(QEMU_LD_PREFIX
    "/usr/aarch64-linux-gnu/"
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

# Set compiler flags for AArch64
set(CMAKE_C_FLAGS
    "--target=aarch64-linux-gnu -mcpu=cortex-a72"
    CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS
    "--target=aarch64-linux-gnu -mcpu=cortex-a72"
    CACHE STRING "C++ flags")

# Set the target environment path
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)

# Adjust the default behavior of the FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Ensure CMake uses the cross-compiler for testing
set(CMAKE_CROSSCOMPILING_EMULATOR "")
