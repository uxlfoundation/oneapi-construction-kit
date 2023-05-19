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
A `CMake script`_ that selectively and recursively copies the contents of one
directory to another. All files are copied except ones that start with
``// Skipped``. The script is used to install kernels from a ComputeAorta build
directory into the corresponding install directory. Kernels that begin with the
skipped comment are stub files used for dependency tracking, and should not be
installed.

The following variables must be defined when this script is invoked (using
``-DOPTION_NAME=value``):

.. cmake:variable:: INPUT_DIR

  Absolute path to the source directory.

.. cmake:variable:: OUTPUT_DIR

  Absolute path to the destination directory.

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

function(recurse_and_copy INPUT OUTPUT)
  file(GLOB children RELATIVE ${INPUT} "${INPUT}/*")
  foreach(child ${children})
    if(IS_DIRECTORY "${INPUT}/${child}")
      file(MAKE_DIRECTORY "${OUTPUT}/${child}")
      recurse_and_copy("${INPUT}/${child}" "${OUTPUT}/${child}")
    else()
      # Check if it's a stub file
      file(STRINGS "${INPUT}/${child}" first_bytes LIMIT_INPUT 10 LIMIT_COUNT 1)
      if(NOT first_bytes STREQUAL "// Skipped")
        file(INSTALL "${INPUT}/${child}" DESTINATION "${OUTPUT}")
      endif()
    endif()
  endforeach()
endfunction()

# Enforce inputs. Since this is a script, it's possible to have these variables
# set to "" if something went wrong in with the caller, so also check for that.
if(NOT DEFINED INPUT_DIR OR INPUT_DIR STREQUAL "")
  message(FATAL_ERROR "INPUT_DIR not set")
endif()

if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "")
  message(FATAL_ERROR "OUTPUT_DIR not set")
endif()

recurse_and_copy("${INPUT_DIR}" "${OUTPUT_DIR}")
