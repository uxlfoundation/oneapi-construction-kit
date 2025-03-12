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
This module finds if `git-clang-format`_ is installed and determines where
the executable script is, version checks are not supported.

.. rubric:: Targets

This module sets the following targets:

git-clang-format
  For use in ``add_custom_{command,target}`` ``COMMAND``'s

.. rubric:: Variables

This module sets the following variables:

.. cmake:variable:: GitClangFormat_FOUND

  Set to ``TRUE`` if found, ``FALSE`` otherwise

.. cmake:variable:: GitClangFormat_EXECUTABLE

  Path to `git-clang-format`_ executable

.. rubric:: Usage

.. code:: cmake

  find_package(GitClangFormat)

.. _git-clang-format:
  https://github.com/llvm/llvm-project/blob/master/clang/tools/clang-format/git-clang-format
#]=======================================================================]

set(GitClangFormat_FOUND FALSE)

# Find all directories in PATH to search for git-clang-format executables.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  foreach(path $ENV{PATH})
    string(REPLACE "\\" "/" path ${path})
    list(APPEND PATH ${path})
  endforeach()
else()
  string(REPLACE ":" ";" PATH $ENV{PATH})
endif()

# Add LLVM install directory to the path since it ships with git-clang-format.
list(APPEND PATH ${LLVM_TOOLS_BINARY_DIR})

foreach(path ${PATH})
  # Find all git-clang-format entries of the following forms:
  # * git-clang-format
  # * git-clang-format.py
  # * git-clang-format-<major>  0 <= major <= 9
  # * git-clang-format-<major>  10 <= major <= 99
  # * git-clang-format-<major>.<minor>
  file(GLOB entries
    ${path}/git-clang-format
    ${path}/git-clang-format.py
    ${path}/git-clang-format-[0-9]
    ${path}/git-clang-format-[0-9][0-9]
    ${path}/git-clang-format-[0-9].[0-9])
  foreach(entry ${entries})
    # Success, found git-clang-format.
    set(GitClangFormat_EXECUTABLE ${entry})
    set(GitClangFormat_FOUND TRUE)
    add_executable(git-clang-format IMPORTED GLOBAL)
    set_target_properties(git-clang-format PROPERTIES
      IMPORTED_LOCATION "${GitClangFormat_EXECUTABLE}")
    break()
  endforeach()
  if(GitClangFormat_FOUND)
    break()
  endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GitClangFormat
  FOUND_VAR GitClangFormat_FOUND
  REQUIRED_VARS GitClangFormat_EXECUTABLE)
mark_as_advanced(GitClangFormat_EXECUTABLE)
