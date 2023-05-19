# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#[=======================================================================[.rst:
ReleaseAssert is a module which adds a new build type to your project. The
``ReleaseAssert`` build type is based on the ``Release`` build type but removes
the command line arguments which disable assertions. To add this build type to
your project, place this file in your project source tree and add the
following to your root ``CMakeLists.txt``.

.. code:: cmake

  include(ReleaseAssert)

Currently the supported platforms are Windows, Linux, and Android.

For Unix Makefiles, Ninja or similar generators you can enable the
``ReleaseAssert`` build type by setting ``CMAKE_BUILD_TYPE=ReleaseAssert``
on the command line or with `ccmake`_ or cmake-gui.

For IDE's such as Visual Studio a ``ReleaseAssert`` build configuration will be
added to the project which can be selected with the GUI interface.

.. _ccmake:
  https://cmake.org/cmake/help/latest/manual/ccmake.1.html
#]=======================================================================]

function(replace_compile_flag From To BuildType)
  if(From)
    string(REPLACE "${From}" "${To}" AsmFlags "${CMAKE_ASM_FLAGS_${BuildType}}")
    string(REPLACE "${From}" "${To}" CFlags "${CMAKE_C_FLAGS_${BuildType}}")
    string(REPLACE "${From}" "${To}" CxxFlags "${CMAKE_CXX_FLAGS_${BuildType}}")
  else()
    set(AsmFlags "${CMAKE_ASM_FLAGS_${BuildType}} ${To}")
    set(CFlags "${CMAKE_C_FLAGS_${BuildType}} ${To}")
    set(CxxFlags "${CMAKE_CXX_FLAGS_${BuildType}} ${To}")
  endif()

  # Remove unnecessary white space after removing flags.
  string(STRIP "${AsmFlags}" AsmFlags)
  string(STRIP "${CFlags}" CFlags)
  string(STRIP "${CxxFlags}" CxxFlags)

  # Get the help string from the cache.
  get_property(AsmFlagsHelpString
    CACHE CMAKE_ASM_FLAGS_${BuildType} PROPERTY HELPSTRING)
  get_property(CFlagsHelpString
    CACHE CMAKE_C_FLAGS_${BuildType} PROPERTY HELPSTRING)
  get_property(CxxFlagsHelpString
    CACHE CMAKE_CXX_FLAGS_${BuildType} PROPERTY HELPSTRING)

  # Store the updated flags in the cache.
  set(CMAKE_ASM_FLAGS_${BuildType} "${AsmFlags}"
      CACHE STRING "${AsmFlagsHelpString}" FORCE)
  set(CMAKE_C_FLAGS_${BuildType} "${CFlags}"
      CACHE STRING "${CFlagsHelpString}" FORCE)
  set(CMAKE_CXX_FLAGS_${BuildType} "${CxxFlags}"
      CACHE STRING "${CxxFlagsHelpString}" FORCE)
endfunction()

# List of the built-in CMAKE_BUILD_TYPEs.
set(BuildTypes MINSIZEREL RELEASE RELWITHDEBINFO)

# If ComputeAorta is the root CMake project copy the build flags from Release
# build type as the basis of the ReleaseAssert build type.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(CMAKE_ASM_FLAGS_RELEASEASSERT ${CMAKE_ASM_FLAGS_RELEASE}
    CACHE STRING "" FORCE)
  set(CMAKE_C_FLAGS_RELEASEASSERT "${CMAKE_C_FLAGS_RELEASE}"
    CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASEASSERT "${CMAKE_CXX_FLAGS_RELEASE}"
    CACHE STRING "" FORCE)

  list(APPEND BuildTypes RELEASEASSERT)
endif()

# When LLVM has enabled assertions strip NDEBUG from the flags of all build
# types since this effects the LLVM headers causing compile errors. This has
# the effect of enabling assertions if LLVM enabled them.
if(LLVM_ENABLE_ASSERTIONS)
  message(STATUS
    "ComputeAorta Enable assertions - inherited from LLVM")
  # The build type is based upon the Release build type with the define NDEBUG
  # flag replaced by the undefine NDEBUG flag.
  foreach(BuildType ${BuildTypes})
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      replace_compile_flag("-DNDEBUG" "-UNDEBUG" ${BuildType})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      replace_compile_flag("/DNDEBUG" "" ${BuildType})
      replace_compile_flag("/D NDEBUG" "" ${BuildType})
    else()
      message(FATAL_ERROR
        "ReleaseAssert unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
  endforeach()
else()
  # `LLVM_ENABLE_ASSERTIONS` is set by default in Debug builds. It's possible
  # that a user did a Release build with `CXXFLAGS=-DNDEBUG` but that's
  # undetectable from the llvm cmake module or llvm-config binary.
  # Without the llvm installation exposing that information there's nothing we
  # can do but hope
  message(STATUS
    "ComputeAorta Disable assertions - inherited from LLVM")
  foreach(BuildType ${BuildTypes})
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      replace_compile_flag("-UNDEBUG" "-DNDEBUG" ${BuildType})
      replace_compile_flag("" "-DNDEBUG" ${BuildType})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      replace_compile_flag("" "/DNDEBUG" ${BuildType})
    else()
      message(FATAL_ERROR
        "ReleaseAssert unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
  endforeach()
endif()

# Don't add the custom build types if ComputeAorta is not the root CMake
# project, this is so we don't clobber the root project settings.
if(NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  return()
endif()

# Don't add ReleaseAssert to the configuration types if CMake does not
# populate the list, this avoid bugs for non IDE builds.
if(NOT "${CMAKE_CONFIGURATION_TYPES}" STREQUAL "")
  # To integrate the ReleaseAssert build type into IDE build configurations we
  # must append ReleaseAssert to CMAKE_CONFIGURATION_TYPES. This should only
  # be done once otherwise ReleaseAssert will be appended every time CMake is
  # configured which can cause side effects.
  list(APPEND CMAKE_CONFIGURATION_TYPES ReleaseAssert)
  list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES}
    CACHE STRING "" FORCE)
endif()

# The complete set of flags is setup for the ReleaseAssert build type.
# Flags which do not enable preprocessor definitions disabling assertions,
# such as linker flags, are copies of the Release build type flags.
set(CMAKE_ASM_FLAGS_RELEASEASSERT
  ${CMAKE_ASM_FLAGS_RELEASEASSERT} CACHE STRING
  "Flags used by the assembler during release assert builds." FORCE)
set(CMAKE_C_FLAGS_RELEASEASSERT
  ${CMAKE_C_FLAGS_RELEASEASSERT} CACHE STRING
  "Flags used by the compiler during release assert builds." FORCE)
set(CMAKE_CXX_FLAGS_RELEASEASSERT
  ${CMAKE_CXX_FLAGS_RELEASEASSERT} CACHE STRING
  "Flags used by the compiler during release assert builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASEASSERT
  ${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING
  "Flags used by the linker during release assert builds." FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_RELEASEASSERT
  ${CMAKE_MODULE_LINKER_FLAGS_RELEASE} CACHE STRING
  "Flags used by the linker during release assert builds." FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASEASSERT
  ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING
  "Flags used by the linker during release assert builds." FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_RELEASEASSERT
  ${CMAKE_STATIC_LINKER_FLAGS_RELEASE} CACHE STRING
  "Flags used by the linker during release assert builds." FORCE)

if(CMAKE_BUILD_TYPE)
  # Finally update the documentation string for CMAKE_BUILD_TYPE to include
  # ReleaseAssert in the list of supported build types.
  get_property(BuildTypeHelp CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING)
  if(NOT BuildTypeHelp MATCHES ReleaseAssert)
    string(REPLACE "." " ReleaseAssert." BuildTypeHelp "${BuildTypeHelp}")
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
      CACHE STRING "${BuildTypeHelp}" FORCE)
  endif()
endif()
