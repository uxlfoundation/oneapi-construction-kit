# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# QNX_HOST and QNX_TARGET are set by qnxsdp-env.sh and required by qcc
if(NOT DEFINED ENV{QNX_HOST} OR NOT DEFINED ENV{QNX_TARGET})
  message(FATAL_ERROR "QNX environment not set up. Run:
    $ source path/to/qnx700/qnxsdp-env.sh")
endif()

set(CMAKE_SYSTEM_VERSION 7)
set(QNX_BINDIR $ENV{QNX_HOST}/usr/bin)

# Set QNX to x86_64
set(CMAKE_C_FLAGS_INIT "-Vgcc_ntox86_64")
set(CMAKE_CXX_FLAGS_INIT "-Vgcc_ntox86_64 -lang-c++")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  message(FATAL_ERROR "Cross-compiling for QNX is not supported on Windows")
endif()

set(CMAKE_SYSTEM_NAME QNX
  CACHE STRING "operating system" FORCE)
set(CMAKE_SYSTEM_PROCESSOR x86_64
  CACHE STRING "processor architecture" FORCE)

# The qnxsdp-env.sh script adds QNX locations to the PATH, so qcc is easy to
# find
find_program(CMAKE_C_COMPILER NAMES "qcc")
find_program(CMAKE_CXX_COMPILER NAMES "q++")

set(CMAKE_AR "${QNX_BINDIR}/x86_64-pc-nto-qnx7.0.0-ar"
  CACHE PATH "archive" FORCE)
set(CMAKE_LINKER "${QNX_BINDIR}/x86_64-pc-nto-qnx7.0.0-ld"
  CACHE PATH "linker" FORCE)
set(CMAKE_NM "${QNX_BINDIR}/x86_64-pc-nto-qnx7.0.0-nm"
  CACHE PATH "nm" FORCE)
set(CMAKE_OBJCOPY "${QNX_BINDIR}/x86_64-pc-nto-qnx7.0.0-objcopy"
  CACHE PATH "objcopy" FORCE)
set(CMAKE_OBJDUMP "${QNX_BINDIR}/x86_64-pc-nto-qnx7.0.0-objdump"
  CACHE PATH "objdump" FORCE)
set(CMAKE_STRIP "${QNX_BINDIR}/x86_64-pc-nto-qnx7.0.0-strip"
  CACHE PATH "strip" FORCE)
set(CMAKE_RANLIB "${QNX_BINDIR}/x86_64-pc-nto-qnx7.0.0-ranlib"
  CACHE PATH "ranlib" FORCE)

set(CMAKE_FIND_ROOT_PATH $ENV{QNX_HOST})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> \
<LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> ${LINKER_LIBS}"
  CACHE STRING "Linker command line" FORCE)
