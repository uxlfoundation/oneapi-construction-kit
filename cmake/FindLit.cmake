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
This module finds if `lit`_ is installed and determines where the executable
is.

.. rubric:: Targets

This module sets the following targets:

lit
  For use in ``add_custom_{command,target}`` ``COMMAND``'s

.. rubric:: Variables

This module sets the following variables:

.. cmake:variable:: Lit_FOUND

  Set to ``TRUE`` if found, ``FALSE`` otherwise

.. cmake:variable:: Lit_EXECUTABLE

  Path to `lit`_ executable

.. rubric:: Usage

.. code:: cmake

  find_package(Lit)

.. _lit:
  https://llvm.org/docs/CommandGuide/lit.html
#]=======================================================================]

set(Lit_FOUND FALSE)

find_program(Lit_EXECUTABLE NAMES
  lit${CMAKE_EXECUTABLE_SUFFIX}
  llvm-lit${CMAKE_EXECUTABLE_SUFFIX})
if(NOT Lit_EXECUTABLE STREQUAL Lit_EXECUTABLE-NOTFOUND)
  set(Lit_FOUND TRUE)
  # Don't add the lit target again if it already exists.
  if(NOT TARGET lit)
    add_executable(lit IMPORTED GLOBAL)
    set_target_properties(lit PROPERTIES
      IMPORTED_LOCATION ${Lit_EXECUTABLE})
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lit
  FOUND_VAR Lit_FOUND REQUIRED_VARS Lit_EXECUTABLE)
mark_as_advanced(Lit_EXECUTABLE)
