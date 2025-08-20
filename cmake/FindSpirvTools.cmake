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
This module searches for `SPIRV-Tools`_ on the systems ``PATH`` or using the
``VULKAN_SDK`` environment variable. Multiple components can be specified to
search for specific tools provided by `SPIRV-Tools`_.

.. seealso::
  Depends on :doc:`/cmake/CAPlatform` for ``CA_HOST_EXECUTABLE_SUFFIX``.

.. rubric:: Targets

This module adds the following targets:

spirv::${component}
  For each specified component

.. rubric:: Variables

This module adds the following variables:

.. cmake:variable:: SpirvTools_FOUND

  Set to ``TRUE`` if all components are found, ``FALSE`` otherwise

.. cmake:variable:: SpirvTools_${component}_EXECUTABLE

  Path to the specifed component

.. rubric:: Usage

.. code:: cmake

  # Finds the spirv-as standalone assembler
  find_package(SpirvTools COMPONENTS spirv-as)

.. _SPIRV-Tools:
  https://github.com/KhronosGroup/SPIRV-Tools
#]=======================================================================]

# Remove `required` from current scope to then set it implicitly in the
# `list(APPEND required ...)` call below.
set(required)

include(CAPlatform)  # Bring in CA_HOST_EXECUTABLE_SUFFIX.

foreach(component ${SpirvTools_FIND_COMPONENTS})
  set(SpirvTools_${component}_FOUND FALSE)
  list(APPEND required SpirvTools_${component}_EXECUTABLE)

  # If the component executable variable has not been set, search for it.
  if(DEFINED SpirvTools_${component}_EXECUTABLE AND
      EXISTS SpirvTools_${component}_EXECUTABLE)
    set(SpirvTools_${component}_FOUND TRUE)
  else()
    find_program(SpirvTools_${component}_EXECUTABLE
      ${component}${CA_HOST_EXECUTABLE_SUFFIX}
      PATHS ENV VULKAN_SDK PATH_SUFFIXES bin)
  endif()

  if(SpirvTools_${component}_EXECUTABLE STREQUAL
      SpirvTools_${component}_EXECUTABLE-NOTFOUND)
    message(WARNING "${component} was not found, set "
      "SpirvTools_${component}_EXECUTABLE manually whilst reconfiguring CMake.")
    continue()  # Search for the next component.
  endif()

  set(SpirvTools_${component}_FOUND TRUE)

  set(target spirv::${component})
  if(NOT TARGET ${target})
    add_executable(${target} IMPORTED GLOBAL)
    set_target_properties(${target} PROPERTIES
      IMPORTED_LOCATION ${SpirvTools_${component}_EXECUTABLE})
  endif()
endforeach()

set(SpirvTools_FOUND TRUE)
foreach(component ${SpirvTools_FIND_COMPONENTS})
  if(NOT SpirvTools_${component}_FOUND)
    set(SpirvTools_FOUND FALSE)
    break()
  endif()
endforeach()

if(SpirvTools_FOUND)
  list(GET required 0 SpirvTools_DIR)
  get_filename_component(SpirvTools_DIR ${SpirvTools_DIR} DIRECTORY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SpirvTools
  REQUIRED_VARS ${required} HANDLE_COMPONENTS)
mark_as_advanced(${required})
