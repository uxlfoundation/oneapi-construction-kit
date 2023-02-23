# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  message(FATAL_ERROR "eMCOS cross compile is only supported on Windows!")
endif()

set(ARM_TOOLCHAIN_ROOT "" CACHE PATH "Path to ARM toolchain root directory.")
set(EMCOS_BUILD_DIR "" CACHE PATH "Path to an eMCOS build.")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARM_TOOLCHAIN_ROOT EMCOS_BUILD_DIR)

if(NOT ARM_TOOLCHAIN_ROOT)
  message(FATAL_ERROR
    "To cross compile for eMCOS you must set ARM_TOOLCHAIN_ROOT to your ARM \
toolchain install directory. This is probably a directory within your ebinder \
install, and called something like ARMCompiler[version-number].")
endif()

if(NOT EMCOS_BUILD_DIR)
  message(FATAL_ERROR
    "To cross compile for eMCOS EMCOS_BUILD_DIR must be set to point at an eMCOS
build. Note that this must be the top level build directory, something like
/path/to/ebinder-workspace/emcos-project/build.")
endif()

# Default to 1GB memory size, this can be overriden.
set(EMCOS_TOTAL_MEMORY_SIZE "1073741824" CACHE STRING
    "Size in bytes of EMCOS device's available memory.")
# Default to 4k page size, this can be overriden.
set(EMCOS_PAGE_SIZE "4096" CACHE STRING
    "Size in bytes of EMCOS device's page size.")

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/emcos_device_info.h.in
  ${PROJECT_BINARY_DIR}/include/emcos/emcos_device_info.h)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/platform/emcos/cmake/")

set(CMAKE_SYSTEM_NAME Linux
  CACHE STRING "operating system" FORCE)
set(CMAKE_SYSTEM_ARCH "armv8-a"
  CACHE STRING "system architecture" FORCE)
set(CMAKE_SYSTEM_PROCESSOR "aarch64"
  CACHE STRING "system processor model" FORCE)

set(CMAKE_C_COMPILER "${ARM_TOOLCHAIN_ROOT}/bin/armclang.exe")
set(CMAKE_CXX_COMPILER "${ARM_TOOLCHAIN_ROOT}/bin/armclang.exe")
set(CMAKE_AR "${ARM_TOOLCHAIN_ROOT}/bin/armar.exe"
  CACHE PATH "archive" FORCE)
set(CMAKE_LINKER "${ARM_TOOLCHAIN_ROOT}/bin/armlink.exe"
  CACHE PATH "linker" FORCE)

set(EMCOS_DEFINES
  "-D__MCOS_POSIX__=1 \
  -D_ARM_LIBCPP_EXTERNAL_THREADS")

set(EMCOS_COMPILER_FLAGS
  "--target=aarch64-arm-none-eabi \
  -mlittle-endian")

set(EMCOS_INCLUDES
  "-I\"${EMCOS_BUILD_DIR}/parts/include\" \
  -I\"${EMCOS_BUILD_DIR}/parts/include_posix/\" \
  -I\"${EMCOS_BUILD_DIR}/parts/config/\" \
  -I\"${PROJECT_BINARY_DIR}/include\"")

set(CMAKE_CXX_FLAGS
  "${EMCOS_COMPILER_FLAGS} ${EMCOS_DEFINES} ${EMCOS_INCLUDES}")
set(CMAKE_CXX_COMPILE_OBJECT
  "<CMAKE_CXX_COMPILER> -c -x c++ -std=gnu++11 <DEFINES> <INCLUDES> <FLAGS> -o \
  <OBJECT> <SOURCE>")

set(CMAKE_C_FLAGS "${EMCOS_COMPILER_FLAGS} ${EMCOS_DEFINES} ${EMCOS_INCLUDES}")
set(CMAKE_C_COMPILE_OBJECT
  "<CMAKE_C_COMPILER> -c -x c -std=c99 <DEFINES> <INCLUDES> <FLAGS> -o \
  <OBJECT> <SOURCE>")

# Disable warnings for unused command line arguments because some flags are on
# by default for armclang that aren't default for other compilers, like
# -ffunction-sections, which cause those warnings to pop up when the flags are
# passed.
set(CMAKE_ASM_FLAGS "${EMCOS_COMPILER_FLAGS} -Wno-unused-command-line-argument")

set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> --create <TARGET> <OBJECTS>")
set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> --create <TARGET> <OBJECTS>")
