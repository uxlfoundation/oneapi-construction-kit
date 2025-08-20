# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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
if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  if(LLVM_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
  endif()
endif()
