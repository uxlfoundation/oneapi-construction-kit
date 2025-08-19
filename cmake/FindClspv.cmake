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
This module finds if `clspv`_ is installed and determines where the executable
is.

.. rubric:: Targets

This module sets the following targets:

clspv
  For use in ``add_custom_{command,target}`` ``COMMAND``'s

.. rubric:: Variables

This module sets the following variables:

.. cmake:variable:: Clspv_FOUND

  Set to ``TRUE`` if found, ``FALSE`` otherwise

.. cmake:variable:: Clspv_EXECUTABLE

  Path to `clspv`_ executable.

.. rubric:: Usage

.. code:: cmake

  find_package(Clspv)

.. _clspv:
  https://github.com/google/clspv
#]=======================================================================]

set(Clspv_FOUND FALSE)

find_program(Clspv_EXECUTABLE clspv)
if(NOT Clspv_EXECUTABLE STREQUAL Clspv_EXECUTABLE-NOTFOUND)
  set(Clspv_FOUND TRUE)
  if(NOT TARGET clspv)  # Don't add the clspv target again if it already exists.
    add_executable(clspv IMPORTED GLOBAL)
    set_target_properties(clspv PROPERTIES IMPORTED_LOCATION ${Clspv_EXECUTABLE})
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Clspv
  FOUND_VAR Clspv_FOUND REQUIRED_VARS Clspv_EXECUTABLE)
mark_as_advanced(Clspv_EXECUTABLE)
