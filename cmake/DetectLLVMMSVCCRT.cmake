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
Module for detect which Windows CRT LLVM was built against, and use that for
our ComputeAorta builds.

Works by querying the CRT that was used to build ``LLVMCore.lib``, and then
set our CRT to be the same as the one LLVM used. Allowing us to mix build
types from LLVM/ComputeAorta.

.. important::
  This module is guarded with a check for ``MSVC`` so it only applies to
  Windows builds.
#]=======================================================================]

# If we are compiling with MSVC we detect the CRT LLVM was built for.
if(NOT MSVC)
  return()
endif()

# LLVM 18 uses the CMake default setting, which depends on the mode LLVM
# was built in, not the mode we were built in.
if(LLVM_VERSION_MAJOR GREATER_EQUAL 18)
  if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    if(LLVM_BUILD_TYPE STREQUAL "Debug")
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
    else()
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    endif()
  endif()
  return()
endif()

# Get the directory of cl.exe
get_filename_component(tools_dir "${CMAKE_C_COMPILER}" DIRECTORY)

# Find the dumpbin.exe executable in the directory of cl.exe
find_program(dumpbin "dumpbin.exe" PATHS "${tools_dir}" NO_DEFAULT_PATH)

if("${dumpbin}" STREQUAL "dumpbin-NOTFOUND")
  message(WARNING "Could not detect which CRT LLVM was built against - "
                  "could not find 'dumpbin.exe'.")
  return()
endif()

# Get the location in the file-system of LLVMCore.lib
get_target_property(llvmcore LLVMCore LOCATION)

if("${llvmcore}" STREQUAL "llvmcore-NOTFOUND")
  message(WARNING "Could not detect which CRT LLVM was built against - "
                  "could not find location of 'LLVMCore.lib'.")
  return()
endif()

# Get the directives that LLVMCore.lib contains
execute_process(COMMAND "${dumpbin}" "/DIRECTIVES" "${llvmcore}"
  OUTPUT_VARIABLE output)

# Find the first directive specifying what CRT to use
string(FIND "${output}" "/FAILIFMISMATCH:RuntimeLibrary=" position)

# Strip away everything but the directive we want to examine
string(SUBSTRING "${output}" ${position} 128 output)

# Remove the directive prefix which we don't need
string(REPLACE "/FAILIFMISMATCH:RuntimeLibrary=" "" output "${output}")

# Get the position of the '_' character that breaks the CRT from all else
string(FIND "${output}" "_" position)

# Substring output to be one of the four CRT values: MDd MD MTd MT
string(SUBSTRING "${output}" 0 ${position} output)

# Set all possible CMAKE_BUILD_TYPE's to the CRT that LLVM was linked against
set(LLVM_USE_CRT_DEBUG "${output}")
set(LLVM_USE_CRT_RELWITHDEBINFO "${output}")
set(LLVM_USE_CRT_MINSIZEREL "${output}")
set(LLVM_USE_CRT_RELEASEASSERT "${output}")
set(LLVM_USE_CRT_RELEASE "${output}")

# Include the LLVM cmake module to choose the correct CRT for our libraries
include(ChooseMSVCCRT)
