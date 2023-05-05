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
A `CMake script`_ for calling :cmake-command:`configure_file`, to be invoked
using ``-P`` as part of an internal :cmake-variable:`CMAKE_COMMAND`.

The script is required to be passed the following argument variables:

.. cmake:variable:: INPUT

  Input file to be configured.

.. cmake:variable:: OUTPUT

  Output file produced by configuration operation.

.. _CMake script:
  https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#scripts
#]=======================================================================]

configure_file(${INPUT} ${OUTPUT})
