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
Module implementing helper macros for declaring options in the ComputeAorta
project under different conditions. This provides a consistent interface across
the various layers of the project from Core implementations to API unit tests.

To bring the option macros exposed by this module into ``CMakeLists.txt`` it
must be included like so:

.. code:: cmake

  include(CAOption)

The following :cmake-command:`CMake macros<macro>` are defined by the module:
#]=======================================================================]


#[=======================================================================[.rst:
.. cmake:command:: ca_option

  A CMake macro which handles all the different option variants we use in the
  ComputeAorta project. It will output a CMake message when the option is set
  to a value other than the default provided value.

  There are a few cases the deprecated option logic has to handle:

  * If no deprecated option is provided, do the normal case (declare an option).
  * If a deprecated option is provided:

    * We check if the CMake command line argument contained a ``-D`` for both
      the original option and the deprecated one, and error if so.
    * We check if just the deprecated option is set, and give a deprecation
      warning to use the replacement option.
    * Clear any cached version of option (we need to be able to differentiate
      between CMake command line ``-D`` and cached versions, and since there is
      no way to do this we do not record the actual option in the cache, and
      instead use ``${CA_OPTION_<OPTION>}``).
    * Set the variable for option to be the value we stored in the cache.
    * Clear any previously cached value for the deprecated option. The last
      known value for the option was read from the cache and stored into our
      new cached variable anyway.

  Implemented by wrapping CMake :cmake-command:`option` for ``BOOL`` types, and
  for other types using :cmake-command:`set` to write a default ``CACHE``
  value, outputting a message rather than overwriting if the value is
  non-default.

  Arguments:
    * ``name`` - The name of the option to declare.
    * ``type`` - The type of option (one of ``FILEPATH``, ``PATH``, ``STRING``,
      ``BOOL``, ``INTERNAL``).
    * ``description`` - The string description of what the option is.
    * ``default`` - The default value of the option.
#]=======================================================================]
macro(ca_option name type description default)
  # Setup the main option of this macro.
  if(${type} STREQUAL "BOOL")
    option(${name} ${description} ${default})
    message(STATUS "oneAPI Construction Kit ${description}: ${${name}}")
  else()
    set(${name} ${default} CACHE ${type} "${description}")
    if(NOT "${${name}}" STREQUAL "${default}")
      message(STATUS "oneAPI Construction Kit ${description}: ${${name}}")
    endif()
  endif()
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: ca_option_required

  A CMake macro which declares an option that is required to be set for the
  ComputeAorta project. If the value of name is equal to the default, this
  macro will error out as we are requiring the option to be set to a value
  other than the default.

  Arguments:
    * ``name`` - The name of the option to declare.
    * ``type`` - The type of option (one of ``FILEPATH``, ``PATH``, ``STRING``,
      ``BOOL``, ``INTERNAL``).
    * ``description`` - The string description of what the option is.
    * ``default`` - The default value of the option.
#]=======================================================================]
macro(ca_option_required name type description default)
  # Setup the option like normal.
  ca_option(${name} ${type} "${description}" "${default}")

  if(NOT DEFINED ${name})
    message(FATAL_ERROR "The option '${name}' is required to be defined.")
  elseif("${name}" STREQUAL "${default}")
    message(FATAL_ERROR "The option '${name}' is required to not be set to "
      "its default value of '${default}'.")
  endif()
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: ca_option_windows

  A CMake macro which declares an option that is only available for Windows.

  Arguments:
    * ``name`` - The name of the option to declare.
    * ``type`` - The type of option (one of ``FILEPATH``, ``PATH``, ``STRING``,
      ``BOOL``, ``INTERNAL``).
    * ``description`` - The string description of what the option is.
    * ``default`` - The default value of the option.
#]=======================================================================]
macro(ca_option_windows name type description default)
  if(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    # Then setup the option like normal.
    ca_option(${name} ${type} "${description}" "${default}")
  endif()
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: ca_option_linux

  A CMake macro which declares an option that is only available for Linux.

  Arguments:
    * ``name`` - The name of the option to declare.
    * ``type`` - The type of option (one of ``FILEPATH``, ``PATH``, ``STRING``,
      ``BOOL``, ``INTERNAL``).
    * ``description`` - The string description of what the option is.
    * ``default`` - The default value of the option.
#]=======================================================================]
macro(ca_option_linux name type description default)
  if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    # Then setup the option like normal.
    ca_option(${name} ${type} "${description}" "${default}")
  endif()
endmacro()

#[=======================================================================[.rst:
.. cmake:command:: ca_immutable_option

  A CMake macro which declares an option that cannot be changed once set.
  When an option controls the default values (and sometimes existence) of other
  options, then it is impossible for CMake to track the correct values of the
  dependent options across re-configurations. Preventing the root option from
  being changed prevents the build directory from going into an invalid state.

  Arguments:
    * ``target`` Library or executable target to link OpenCL into.
    * ``name`` - The name of the option to declare.
    * ``type`` - The type of option (only ``BOOL`` is currently supported).
    * ``description`` - The string description of what the option is.
    * ``default`` - The default value of the option.
#]=======================================================================]
macro(ca_immutable_option name type description default)
  if(NOT "BOOL" STREQUAL "${type}")
    message(FATAL_ERROR "ca_immutable_option only supports BOOL types")
  endif()
  ca_option(${name} ${type} "${description}" "${default}")

  # Create a "name_GUARD" variable to store a copy of the option's value. Since
  # "CA_TRUE" and "CA_FALSE" are both evaluated to TRUE, the guard variable is
  # only created once.
  if(${name}_GUARD)
    if(${name})
      if(NOT ${name}_GUARD STREQUAL "CA_TRUE")
        message(FATAL_ERROR
          "Changing the value of ${name} is not supported. Please configure "
          "a new build directory.")
      endif()
    else()
      if(NOT ${name}_GUARD STREQUAL "CA_FALSE")
        message(FATAL_ERROR
          "Changing the value of ${name} is not supported. Please configure "
          "a new build directory.")
      endif()
    endif()
  else()
    if(${name})
      set("${name}_GUARD" "CA_TRUE" CACHE INTERNAL
        "Guard variable for ${name}")
    else()
      set("${name}_GUARD" "CA_FALSE" CACHE INTERNAL
        "Guard variable for ${name}")
    endif()
  endif()
endmacro()
