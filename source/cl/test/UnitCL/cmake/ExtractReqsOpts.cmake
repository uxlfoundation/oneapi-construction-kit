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
A file with the :cmake:command:`extract_reqs_opts` function.
#]=======================================================================]
include(CMakeParseArguments)

#[=======================================================================[.rst:
.. cmake:command:: extract_reqs_opts

The ``extract_reqs_opts()`` function extracts the ``REQUIRES`` and ``OPTIONS``
comment fields from a ``.cl`` file.

Arguments:
  * ``INPUT_FILE`` - A ``.cl`` file that may have requirements and compilation
    options in comments (required).
  * ``REQS_VAR`` - The name of a variable in parent scope that will be
    populated with a ``;``-separated list of requirements extracted from the
    input file (required).
  * ``DEFS_VAR`` - The name of a variable in parent scope that will be
    populated with a ``;``-separated list of compile definition options
    extracted from the input file (optional).
  * ``CL_STD_VAR`` - The name of a variable in parent scope that will be
    populated with the OpenCL C standard required to compile the input file
    (optional).
  * ``CLC_OPTS_VAR`` - The name of a variable in parent scope that will be
    populated with a ``;``-separated list of CLC compilation options extracted
    from the input file (optional).
  * ``SPIR_OPTS_VAR`` - The name of a variable in parent scope that will be
    populated with a ``;``-separated list of Clang SPIR compilation options
    extracted from the input file (optional).
  * ``SPIRV_OPTS_VAR`` - The name of a variable in parent scope that will be
    populated with a ``;``-separated list of Clang SPIR-V compilation options
    extracted from the input file (optional).

Example:

  extract_reqs_opts(
    INPUT_FILE <filename>
    REQS_VAR <requirements list variable>
    [DEFS_VAR <definitions list variable>]
    [CL_STD_VAR <opencl c standard variable>]
    [CLC_OPTS_VAR <clc options list variable>]
    [SPIR_OPTS_VAR <clang spir options list variable>]
    [SPIRV_OPTS_VAR <clang spir-v options list variable>]
  )
#]=======================================================================]
function(extract_reqs_opts)
  set(single_args
    INPUT_FILE
    REQS_VAR
    DEFS_VAR
    CL_STD_VAR
    CLC_OPTS_VAR
    SPIR_OPTS_VAR
    SPIRV_OPTS_VAR
  )
  cmake_parse_arguments(REQS_OPTS "" "${single_args}" "" ${ARGN})

  # Handles comments for clc requirements in this format:
  # // REQUIRES: noclc; double; half
  # Individual options must be semicolon-separated due to CMake's space
  # escaping behavior
  set(CLC_REQUIREMENTS_REGEX "^ *// +REQUIRES: *(.*)$")

  # Get the comment line with requirements
  file(STRINGS ${REQS_OPTS_INPUT_FILE} CLC_FILE_REQUIREMENTS_LINE
    LIMIT_COUNT 1
    REGEX ${CLC_REQUIREMENTS_REGEX})

  # And trim to the actual requirements, because file() doesn't set
  # ${CMAKE_MATCH_1}
  string(REGEX MATCH ${CLC_REQUIREMENTS_REGEX}
    CLC_FILE_REQUIREMENTS_LINE "${CLC_FILE_REQUIREMENTS_LINE}")
  set(CLC_FILE_REQUIREMENTS ${CMAKE_MATCH_1})

  # Turn the space separated list into a semi-colon separated list so CMake can
  # iterate over it
  string(REPLACE " " ";" CLC_FILE_REQUIREMENTS "${CLC_FILE_REQUIREMENTS}")

  # Check for unkown requirements
  set(known_requirements
      "noclc" "nospir" "nospirv" "double" "half" "images" "parameters")
  foreach(file_req ${CLC_FILE_REQUIREMENTS})
    set(req_found FALSE)
      foreach(known_req ${known_requirements})
        if(file_req STREQUAL known_req)
          set(req_found TRUE)
          break()
        endif()
      endforeach()
      if(NOT req_found)
        message(FATAL_ERROR
          "Unknown CL kernel requirement '${file_req}' in file '${INPUT_FILE}'")
      endif()
  endforeach()

  # Finally, store the requirements in the parent scope
  set(${REQS_OPTS_REQS_VAR} ${CLC_FILE_REQUIREMENTS} PARENT_SCOPE)

  # Handles comments with custom clc or clang options in this format:
  # // CLC OPTIONS: -cl-mad-enable;-D DEF1=7
  # Or with quotes:
  # // CLC OPTIONS: "-cl-mad-enable";"-D DEF1=7"
  # Individual options must be semicolon-separated due to CMake's space
  # escaping behavior
  set(DEFINITIONS_REGEX "^ *// *DEFINITIONS: *(.*)$")
  set(CL_STD_REGEX "^ *// *CL_STD: ([0-9]+.[0-9]+)$")
  set(CLC_OPTIONS_REGEX "^ *// *CLC +OPTIONS: *(.*)$")
  set(SPIR_OPTIONS_REGEX "^ *// *SPIR +OPTIONS: *(.*)$")
  set(SPIRV_OPTIONS_REGEX "^ *// *SPIRV +OPTIONS: *(.*)$")

  macro(extract_options_line_from_file INPUT_FILE OPTIONS_REGEX OPTIONS_STRING)
    # Get the comment line with options
    file(STRINGS ${INPUT_FILE} CLC_FILE_OPTIONS_LINE
            LIMIT_COUNT 1
            REGEX ${OPTIONS_REGEX}) # get the comment line
    # And trim to the actual requirements, because file() doesn't set
    # ${CMAKE_MATCH_1}
    string(REGEX MATCH ${OPTIONS_REGEX}
            CLC_FILE_OPTIONS_LINE "${CLC_FILE_OPTIONS_LINE}")
    # ${OPTS_STRING} is passed to execute_process(), which doesn't pass commands
    # through a shell. `"` would normally be removed by a shell, so remove any
    # here instead.
    string(REGEX REPLACE "\"" "" OPTIONS_MATCH_RESULT "${CMAKE_MATCH_1}")
    set(${OPTIONS_STRING} ${OPTIONS_MATCH_RESULT} PARENT_SCOPE)
  endmacro()

  # Extract the specified option lists.
  if(DEFINED REQS_OPTS_DEFS_VAR)
    extract_options_line_from_file(
      "${REQS_OPTS_INPUT_FILE}"
      "${DEFINITIONS_REGEX}"
      ${REQS_OPTS_DEFS_VAR}
    )
  endif()
  if(DEFINED REQS_OPTS_CL_STD_VAR)
    extract_options_line_from_file(
      "${REQS_OPTS_INPUT_FILE}"
      "${CL_STD_REGEX}"
      ${REQS_OPTS_CL_STD_VAR}
    )
  endif()
  if(DEFINED REQS_OPTS_CLC_OPTS_VAR)
    extract_options_line_from_file(
      "${REQS_OPTS_INPUT_FILE}"
      "${CLC_OPTIONS_REGEX}"
      ${REQS_OPTS_CLC_OPTS_VAR}
    )
  endif()
  if(DEFINED REQS_OPTS_SPIR_OPTS_VAR)
    extract_options_line_from_file(
      "${REQS_OPTS_INPUT_FILE}"
      "${SPIR_OPTIONS_REGEX}"
      ${REQS_OPTS_SPIR_OPTS_VAR}
    )
  endif()
  if(DEFINED REQS_OPTS_SPIRV_OPTS_VAR)
    extract_options_line_from_file(
      "${REQS_OPTS_INPUT_FILE}"
      "${SPIRV_OPTIONS_REGEX}"
      ${REQS_OPTS_SPIRV_OPTS_VAR}
    )
  endif()
endfunction()
