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
A `Cmake script`_ for copying OpenCL-C, SPIR, and SPIR-V kernels to a
ComputeAorta build directory. OpenCL-C and SPIR kernels are only copied, but
SPIR-V kernels must also be assembled. However, if a SPIR-V assembly file is a
stub file (i.e., it only exists for dependency tracking reasons), then it
cannot be assembled. This script encapsulates all this logic.

The following variables must be defined when this script is invoked (using
``-DOPTION_NAME=value``):

.. cmake:variable:: INPUT_FILE

  The ``.cl``, ``.bc32``, ``.bc64``, ``.spvasm32``, or ``.spvasm64`` kernel
  that will be copied (and possibly assembled).

.. cmake:variable:: OUTPUT_DIR

  The directory that the kernel files should be copied to.

The following variable may optionally be defined:

.. cmake:variable:: SPIRV_AS_PATH

  Absolute path to ``spirv-as``. If an input file is SPIR-V assembly, then this
  option is required; it is ignored otherwise.

.. warn::
  This script must be run from the ComputeAorta root directory (i.e., with
  ``WORKING_DIRECTORY ${ComputeAorta_SOURCE_DIR}``) so that dependencies can be
  found.

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

# Enforce inputs. Since this is a script, it's possible to have these variables
# set to "" if something went wrong in with the caller, so also check for that.
if(NOT DEFINED INPUT_FILE OR INPUT_FILE STREQUAL "")
  message(FATAL_ERROR "INPUT_FILE not set")
endif()

if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "")
  message(FATAL_ERROR "OUTPUT_DIR not set")
endif()

# Optional: SPIRV_AS_PATH

get_filename_component(input_ext ${INPUT_FILE} LAST_EXT)
get_filename_component(input_name ${INPUT_FILE} NAME_WLE)

# .spvasm files may need to be assembled; everything else is just copied
if(input_ext MATCHES ".spvasm(32|64)")
  # Turn `spvasm{32|64}` into `spv{32|64}`
  string(REGEX REPLACE "asm" "" output_ext ${input_ext})
  set(output_file "${OUTPUT_DIR}/${input_name}${output_ext}")

  # Check if it's a stub file
  file(STRINGS "${INPUT_FILE}" first_bytes LIMIT_INPUT 10 LIMIT_COUNT 1)

  if(first_bytes STREQUAL "// Skipped")
    execute_process(
      COMMAND "cp" ${INPUT_FILE} ${output_file}
      RESULT_VARIABLE cp_result
      ERROR_VARIABLE cp_error)
    if(NOT cp_result EQUAL 0)
      message(FATAL_ERROR
        "cp failed with status '${cp_result}':
        cp ${INPUT_FILE} ${output_file}
        ${cp_error}")
    endif()
  else()
    if(NOT EXISTS "${SPIRV_AS_PATH}")
      message(FATAL_ERROR
        "Cannot assemble '${INPUT_FILE}'; SPIRV_AS_PATH not defined.")
    endif()

    execute_process(
      COMMAND ${SPIRV_AS_PATH} --target-env spv1.0
      -o ${output_file}
      ${INPUT_FILE}
      RESULT_VARIABLE as_result
      ERROR_VARIABLE as_error)

    if(NOT as_result EQUAL 0)
      message(FATAL_ERROR
        "spirv-as failed with status '${as_result}':
        ${SPIRV_AS_PATH} -target-env spv1.0
          -o ${OUTPUT_FILE}
          ${INPUT_FILE}
        ${as_error}")
    endif()
  endif()
else()
  file(COPY ${INPUT_FILE} DESTINATION ${OUTPUT_DIR})
endif()
