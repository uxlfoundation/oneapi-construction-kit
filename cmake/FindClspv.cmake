# Copyright (C) Codeplay Software Limited. All Rights Reserved.
#[=======================================================================[.rst:
This module finds if `clspv`_ is installed and determines where the executable
is.

.. rubric:: Targets

This module sets the following targets:

clspv
  For use in ``add_custom_{command,target}`` ``COMMAND``'s

.. rubric:: Variables

This module sets the following variables:

.. cmake:variable:: Clspv_FOUND

  Set to ``TRUE`` if found, ``FALSE`` otherwise

.. cmake:variable:: Clspv_EXECUTABLE

  Path to `clspv`_ executable.

.. rubric:: Usage

.. code:: cmake

  find_package(Clspv)

.. _clspv:
  https://github.com/google/clspv
#]=======================================================================]

set(Clspv_FOUND FALSE)

find_program(Clspv_EXECUTABLE clspv)
if(NOT Clspv_EXECUTABLE STREQUAL Clspv_EXECUTABLE-NOTFOUND)
  set(Clspv_FOUND TRUE)
  if(NOT TARGET clspv)  # Don't add the clspv target again if it already exists.
    add_executable(clspv IMPORTED GLOBAL)
    set_target_properties(clspv PROPERTIES IMPORTED_LOCATION ${Clspv_EXECUTABLE})
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Clspv
  FOUND_VAR Clspv_FOUND REQUIRED_VARS Clspv_EXECUTABLE)
mark_as_advanced(Clspv_EXECUTABLE)
