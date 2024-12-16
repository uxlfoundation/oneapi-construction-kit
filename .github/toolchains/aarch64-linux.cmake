set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TRIPLE "aarch64-linux-gnu")
find_program(CMAKE_C_COMPILER NAMES
  ${TRIPLE}-gcc
  ${TRIPLE}-gcc-14 ${TRIPLE}-gcc-13 ${TRIPLE}-gcc-12
  ${TRIPLE}-gcc-11 ${TRIPLE}-gcc-10 ${TRIPLE}-gcc-9
  ${TRIPLE}-gcc-8  ${TRIPLE}-gcc-7)
find_program(CMAKE_CXX_COMPILER NAMES
  ${TRIPLE}-g++
  ${TRIPLE}-g++-14 ${TRIPLE}-g++-13 ${TRIPLE}-g++-12
  ${TRIPLE}-g++-11 ${TRIPLE}-g++-10 ${TRIPLE}-g++-9
  ${TRIPLE}-g++-8  ${TRIPLE}-g++-7)
set(PKG_CONFIG_EXECUTABLE ${TRIPLE}-pkg-config)

set(CMAKE_FIND_ROOT_PATH /usr/${TRIPLE})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# vc-intrinsics, used as part of the DPC++ build, uses find_file to find
# the absolute path of a file that it already knows the absolute path of
# in order to get a useful error message if it is not found, but the way
# it does this does not account for find_file's cross compilation
# support.
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_CROSSCOMPILING_EMULATOR qemu-aarch64;-L;/usr/aarch64-linux-gnu)

