# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
.. cmake:variable:: MUX_COMPILER_LIBRARIES

  Variable containing a list of ComputeMux compiler libraries
#]=======================================================================]
# Clear the list of target libraries before inclusion, enables reconfiguring a
# build directory with a different set of targets.
set(MUX_COMPILER_LIBRARIES "" CACHE INTERNAL
  "List of ComputeMux compiler libraries." FORCE)

#[=======================================================================[.rst:
.. cmake:command:: add_mux_compiler_target

  The ``add_mux_compiler_target`` function makes ComputeMux aware of the
  existence of a compiler target implementing the ComputeMux compiler API,
  ensuring that ComputeMux has the required information to properly integrate
  the compiler into the build.

  The following arguments are required:
    * ``target`` - the compiler implementation CMake target.

  The following keyword arguments are available:

  * ``COMPILER_INFO`` - (required) the fully qualified name of a function that
    returns static instance(s) of the compiler::Info object, describing each
    compiler configuration this target supports.
  * ``HEADER_FILE <path>`` - (required) the path to a header file that declares
    the above class or struct.
#]=======================================================================]
function(add_mux_compiler_target target)
  cmake_parse_arguments(args "" "COMPILER_INFO;HEADER_FILE" "" ${ARGN})
  if(args_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "add_mux_compiler_target invalid arguments: ${args_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT args_COMPILER_INFO)
    message(FATAL_ERROR "add_mux_compiler_target COMPILER_INFO not set")
  endif()
  if(args_HEADER_FILE STREQUAL "")
    message(FATAL_ERROR "add_mux_compiler_target HEADER_FILE not set")
  endif()

  # Append to the list of Mux compiler libraries if it's not there already, the
  # cache MUST be updated.
  if(NOT ${target} IN_LIST MUX_COMPILER_LIBRARIES)
    list(APPEND MUX_COMPILER_LIBRARIES ${target})
    set(MUX_COMPILER_LIBRARIES ${MUX_COMPILER_LIBRARIES}
      CACHE INTERNAL "List of ComputeMux compiler libraries.")
  endif()

  # Add COMPILER_INFO and HEADER_FILE as custom properties to the compiler target.
  set_property(TARGET ${target} APPEND PROPERTY COMPILER_INFO_NAMES ${args_COMPILER_INFO})
  set_property(TARGET ${target} APPEND PROPERTY COMPILER_INFO_HEADERS ${args_HEADER_FILE})
endfunction()

# Add in tree compiler targets.
file(GLOB Entries RELATIVE
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
list(REMOVE_ITEM Entries base)
list(REMOVE_ITEM Entries compat)
foreach(Entry ${Entries})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${Entry})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${Entry}/CMakeLists.txt)
      if(CA_MUX_COMPILERS_TO_ENABLE STREQUAL "" OR
        ${Entry} IN_LIST CA_MUX_COMPILERS_TO_ENABLE)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${Entry})
      endif()
    endif()
  endif()
endforeach()

# Add out of tree mux targets.
if(NOT CA_EXTERNAL_MUX_COMPILER_DIRS STREQUAL "")
  foreach(Entry "${CA_EXTERNAL_MUX_COMPILER_DIRS}")
    message(STATUS "Entry: ${Entry}")
    if(NOT IS_DIRECTORY ${Entry})
      message(FATAL_ERROR
        "CA_EXTERNAL_MUX_COMPILER_DIRS entry is not a directory: ${Entry}")
    endif()
    get_filename_component(Name ${Entry} NAME)
    if(CA_MUX_COMPILERS_TO_ENABLE STREQUAL "" OR
      ${Name} IN_LIST CA_MUX_COMPILERS_TO_ENABLE)
      add_subdirectory(${Entry} ${CMAKE_CURRENT_BINARY_DIR}/${Name})
    endif()
  endforeach()
endif()
