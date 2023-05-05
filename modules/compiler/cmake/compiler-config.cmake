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
A `CMake script`_ for creating the ``compilerConfig.h`` header used to provide
an API for selecting a compiler implementation from a Mux target.

Script should be invoked using ``-P`` as part of an internal
:cmake-variable:`CMAKE_COMMAND`. The following CMake input variables are
required to be set:

* :cmake:variable:`COMPILER_INFO_NAMES` - Input variable containing a list of
  fully qualified names to the subclasses of compiler::Info that identify
  compiler implementations.

* :cmake:variable:`COMPILER_INFO_HEADERS` - Input variable containing a list of
  headers to include to obtain the compiler::Info instances.

* :cmake:variable:`COMPILER_CONFIG_SOURCE` - Input variable containing the path
  to the source file to be written to.

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

# Make sure this is actually a list, it may be passed in with space separators
# due to CMake deconstructing the list into a space separated string.
if(NOT ${COMPILER_INFO_NAMES} STREQUAL "")
  string(REPLACE "," ";" COMPILER_INFO_NAMES ${COMPILER_INFO_NAMES})
endif()
if(NOT ${COMPILER_INFO_HEADERS} STREQUAL "")
  string(REPLACE "," ";" COMPILER_INFO_HEADERS ${COMPILER_INFO_HEADERS})
endif()

file(WRITE ${COMPILER_CONFIG_SOURCE} "\
// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the \"License\") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an \"AS IS\" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// AUTO-GENERATED FILE - DO NOT EDIT (CMake will replace it if you do!)
// To make changes to this file update modules/compiler/cmake/compiler-config.cmake

#include <compiler/library.h>

")

file(APPEND ${COMPILER_CONFIG_SOURCE} "\
#include <array>

")

list(LENGTH COMPILER_INFO_NAMES NUM_COMPILER_INFOS)

foreach(INCLUDE ${COMPILER_INFO_HEADERS})
  file(APPEND ${COMPILER_CONFIG_SOURCE} "\
#include <${INCLUDE}>
")
endforeach()

file(APPEND ${COMPILER_CONFIG_SOURCE} "
namespace compiler {
cargo::array_view<const compiler::Info *> compilers() {
")

file(APPEND ${COMPILER_CONFIG_SOURCE} "\
  static std::vector<const compiler::Info *> compilers_list;
  static std::once_flag compilers_initialized;
  std::call_once(compilers_initialized,
    [] {
      auto add_compiler = [](const compiler::Info* info) {
        compilers_list.push_back(info);
      };
")
foreach(COMPILER_INFO ${COMPILER_INFO_NAMES})
  file(APPEND ${COMPILER_CONFIG_SOURCE} "\
      ${COMPILER_INFO}(add_compiler);
")
endforeach()
file(APPEND ${COMPILER_CONFIG_SOURCE} "\
    });
")

file(APPEND ${COMPILER_CONFIG_SOURCE} "\
  return compilers_list;
}
}  // namespace compiler
")
