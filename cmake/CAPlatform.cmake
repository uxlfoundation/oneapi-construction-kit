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
Declares useful variables for the ComputeAorta project based on the host and
target platforms. To access the variables in this module:

.. code:: cmake

  include(CAPlatform)
#]=======================================================================]

if(CAPlatform_INCLUDED)
  return()
endif()
set(CAPlatform_INCLUDED TRUE)

#[=======================================================================[.rst:
.. cmake:variable:: CA_PLATFORM_ANDROID

  Set to ``TRUE`` if our build is targeting an :cmake-variable:`ANDROID`
  system.

.. cmake:variable:: CA_PLATFORM_LINUX

  Set to ``TRUE`` if we are targeting a Linux build by querying
  :cmake-variable:`CMAKE_SYSTEM_NAME`.

.. cmake:variable:: CA_PLATFORM_WINDOWS

  Set to ``TRUE`` if we are targeting a Windows build by querying
  :cmake-variable:`CMAKE_SYSTEM_NAME`.

.. cmake:variable:: CA_PLATFORM_MAC

  Set to ``TRUE`` if we are targeting an Apple platform build by detecting the
  Darwin OS in :cmake-variable:`CMAKE_SYSTEM_NAME`.

.. cmake:variable:: CA_PLATFORM_QNX

  Set to ``TRUE`` if we are targeting a QNX build by querying
  :cmake-variable:`CMAKE_SYSTEM_NAME`.
#]=======================================================================]
# Determine the target platform and set configured variables.
if(ANDROID)
  set(CA_PLATFORM_ANDROID TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CA_PLATFORM_LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CA_PLATFORM_WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CA_PLATFORM_MAC TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "QNX")
  set(CA_PLATFORM_QNX TRUE)
else()
  message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

#[=======================================================================[.rst:
.. cmake:variable:: CA_HOST_EXECUTABLE_SUFFIX

  ``CA_HOST_EXECUTABLE_SUFFIX`` is the executable extension for running
  programs on the host system, irrespective of the target system. It is used
  for programs that will be run as part of the build process, or for programs
  that will be built using :cmake-variable:`CMAKE_EXECUTABLE_SUFFIX`.

  .. seealso::
    The notion of a `CMAKE_HOST_EXECUTABLE_SUFFIX`_ variable has already been
    proposed for CMake, should a future version of CMake include that then that
    could be used instead.

  The suffix is ``.exe`` if and only if the **host** system is Windows.

.. _CMAKE_HOST_EXECUTABLE_SUFFIX:
  https://gitlab.kitware.com/cmake/cmake/issues/17553
#]=======================================================================]
if(CMAKE_HOST_WIN32)
  set(CA_HOST_EXECUTABLE_SUFFIX ".exe")
else()
  set(CA_HOST_EXECUTABLE_SUFFIX "")
endif()
