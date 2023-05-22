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
Utilities to convert binary files to header files at build time using only
CMake in script mode. To use these utilities:

.. code:: cmake

  include(Bin2H)

  add_bin2h_command(my_bin
    ${path_to_input}/file.bin
    ${path_to_output}/file.h)

  add_bin2h_target(my_bin
    ${path_to_input}/file.bin
    ${path_to_output}/file.h)
#]=======================================================================]

#[=======================================================================[.rst:
.. cmake:command:: add_bin2h_command

  The ``add_bin2h_command`` macro creates a custom command which generates a
  header file from a binary file that can be included into source code.
  Additionally the macro specifies that the header file is generated at build
  time, this allows it to be used as a source file for libraries and
  exectuables.

  Arguments:
    * ``variable``: The name of the variable to access the data in the header
      file
    * ``input``: The binary input filepath to be made into a header file
    * ``output``: The header filepath to generate
#]=======================================================================]
function(add_bin2h_command variable input output)
  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
  get_property(HAL_REFSI_TUTORIAL_DIR GLOBAL PROPERTY HAL_REFSI_TUTORIAL_DIR)
  add_custom_command(OUTPUT ${output}
    COMMAND ${CMAKE_COMMAND}
      -DBIN2H_INPUT_FILE:FILEPATH="${input}"
      -DBIN2H_OUTPUT_FILE:FILEPATH="${output}"
      -DBIN2H_VARIABLE_NAME:STRING="${variable}"
      -P ${HAL_REFSI_TUTORIAL_DIR}/cmake/Bin2HScript.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${input} ${HAL_REFSI_TUTORIAL_DIR}/cmake/Bin2HScript.cmake
    COMMENT "Generating H file ${relOut}")
  set_source_files_properties(${output} PROPERTIES GENERATED ON)
endfunction()

#[=======================================================================[.rst:
.. cmake:command:: add_bin2h_target

  The ``add_bin2h_target`` macro creates a custom target which generates a
  header file from a binary file that can be included into source code and use
  the target in CMake dependency tracking.

  Arguments:
    * ``target``: The name of the target and the name of the variable to access
      the data in the header file
    * ``input``: The binary input filepath to be made into a header file
    * ``output``: The header filepath to generate
#]=======================================================================]
function(add_bin2h_target target input output)
  add_bin2h_command(${target} ${input} ${output})
  file(RELATIVE_PATH relOut ${CMAKE_BINARY_DIR} ${output})
  add_custom_target(${target}
    DEPENDS ${output}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating H file ${relOut}")
endfunction()
