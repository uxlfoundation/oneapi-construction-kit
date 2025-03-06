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
This find package module searches for clang tools on the system ``PATH``. In
order to be future proof, the desired tools are specified using the
``COMPONENTS`` keyword and the desired version is specified using the
``VERSION`` keyword.

.. seealso::
  Depends on :doc:`/cmake/CAPlatform` for ``CA_HOST_EXECUTABLE_SUFFIX``.

.. rubric::Targets

This module adds the following targets:

ClangTools::${component}
  For each specifed component

.. rubric:: Variables

This modules adds the following variables:

.. cmake:variable:: ClangTools_FOUND

  Set to ``TRUE`` if all components found, ``FALSE`` otherwise

.. cmake:variable:: ClangTools_${component}_EXECUTABLE

  Path to the specified component

.. rubric:: Usage

.. code:: cmake

  # Discovers Version 16 of clang-format & clang-tidy
  find_package(ClangTools 16 COMPONENTS clang-format clang-tidy)

#]=======================================================================]

list(LENGTH ClangTools_FIND_COMPONENTS components_length)
if(components_length EQUAL 0)
  message(FATAL_ERROR "FindClangTools: COMPONENTS list is empty")
endif()

if(NOT DEFINED ClangTools_FIND_VERSION)
  message(FATAL_ERROR "FindClangTools: VERSION must be specified")
endif()

# Find all directories in PATH to search for tool executables.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  # Loop over PATH entries and normalize directory slashes, the loop is
  # required otherwise list separators get stripped.
  foreach(path $ENV{PATH})
    string(REPLACE "\\" "/" path ${path})
    list(APPEND PATH ${path})
  endforeach()
else()
  string(REPLACE ":" ";" PATH $ENV{PATH})
endif()

set(required ClangTools_DIR)
set(ClangTools_VERSION_STRING ${ClangTools_FIND_VERSION})

include(CAPlatform)  # Bring in CA_HOST_EXECUTABLE_SUFFIX.

foreach(component ${ClangTools_FIND_COMPONENTS})
  if(TARGET ClangTools::${component})
    # Imported target already exists, continue to next component.
    set(ClangTools_${component}_FOUND TRUE)
    continue()
  endif()

  set(ClangTools_${component}_FOUND FALSE)
  list(APPEND required ClangTools_${component}_EXECUTABLE)

  foreach(path ${PATH})
    # Find all clang-format entries of the following forms:
    # * ${component}
    # * ${component}-<major>  0 <= major <= 9
    # * ${component}-<major>  10 <= major <= 99
    # * ${component}-<major>.<minor>
    file(GLOB entries
      ${path}/${component}${CA_HOST_EXECUTABLE_SUFFIX}
      ${path}/${component}-[0-9]${CA_HOST_EXECUTABLE_SUFFIX}
      ${path}/${component}-[0-9][0-9]${CA_HOST_EXECUTABLE_SUFFIX}
      ${path}/${component}-[0-9].[0-9]${CA_HOST_EXECUTABLE_SUFFIX})

    # Check each entry has the desired version.
    foreach(entry ${entries})
      set(ClangTools_${component}_EXECUTABLE ${entry})
      # Query component for its version.
      execute_process(COMMAND ${entry} --version
        OUTPUT_VARIABLE version_string RESULT_VARIABLE result)

      if(result EQUAL 0)
        # Strip all non-version text from the output, we only care about the
        # <major> version component.
        string(REGEX MATCH "[0-9]+" version_string ${version_string})
        if(version_string VERSION_EQUAL ClangTools_FIND_VERSION)
          # Success, found the correct version of the component.
          set(ClangTools_${component}_FOUND TRUE)

          # Create an imported executable target for the found component if one
          # does not already exist.
          if(NOT TARGET ClangTools::${component})
            add_executable(ClangTools::${component} IMPORTED GLOBAL)
            set_target_properties(ClangTools::${component}
              PROPERTIES IMPORTED_LOCATION ${entry})
          endif()
          break()
        endif()
      endif()
    endforeach()
  endforeach()
endforeach()

set(ClangTools_FOUND TRUE)
foreach(component ${ClangTools_FIND_COMPONENTS})
  if(NOT ClangTools_${component}_FOUND)
    set(ClangTools_FOUND FALSE)
    break()
  endif()
endforeach()

if(ClangTools_FOUND)
  list(GET required 1 component_var)
  get_filename_component(ClangTools_DIR ${${component_var}} DIRECTORY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ClangTools HANDLE_COMPONENTS
  REQUIRED_VARS ${required} VERSION_VAR ClangTools_VERSION_STRING)
