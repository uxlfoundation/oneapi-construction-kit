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
Module adding a ``format`` CMake target to automatically format modified source
files using `clang-format`_.

To add the target to a project include this module in the root
``CMakeLists.txt``.

.. code:: cmake

  include(Format)

Users can run the target to format modified files like so:

.. code:: console

  $ ninja format
  [0/1] clang-format modified source files.

.. _clang-format:
  https://clang.llvm.org/docs/ClangFormat.html
#]=======================================================================]

# Don't include this file twice.
if(${GIT_CLANG_FORMAT_INCLUDED})
  return()
endif()
set(GIT_CLANG_FORMAT_INCLUDED TRUE)

find_package(GitClangFormat)
if(NOT PythonInterp_FOUND OR NOT TARGET ClangTools::clang-format OR
    NOT GitClangFormat_FOUND)
  message(WARNING "Dependencies for format target not met, disabled.")
  return()
endif()

# Add format target to clang-format modified source files.
add_custom_target(format
  COMMAND ${PYTHON_EXECUTABLE} ${GitClangFormat_EXECUTABLE}
    --binary ${ClangTools_clang-format_EXECUTABLE} --style=file --force
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMENT "clang-format modified source files.")

# TODO(CA-692): Add support for prettier to format markdown documents, this
# will require finding the prettier tool and scanning git status for editing
# .md files to format.
