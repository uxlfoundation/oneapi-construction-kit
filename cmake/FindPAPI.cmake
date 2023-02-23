# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
A `Find Module`_ for detecting a system install of `PAPI`_, the low level
hardware performance counter API, and its headers.

.. seealso::
  Implemented using `FindPackageHandleStandardArgs`_

.. rubric:: Targets

This module adds the following targets:

papi::papi

.. rubric:: Variables

This module adds the following variables:

.. cmake:variable:: PAPI

  Path to the found PAPI library.

.. cmake:variable:: PAPI_INCLUDE

  Path to the PAPI headers.

.. cmake:variable:: PAPI_FOUND

  Boolean, true only if the module was able to locate a PAPI install.

Usage
-----

To use load this module either include it or use CMake `find_package`_.

.. code:: cmake

  include(FindPAPI)

.. code:: cmake

  find_package(PAPI)

.. _Find Module:
  https://cmake.org/cmake/help/latest/manual/cmake-developer.7.html#find-modules
.. _FindPackageHandleStandardArgs:
  https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
.. _PAPI:
  https://icl.utk.edu/papi/
.. _find_package:
  https://cmake.org/cmake/help/latest/command/find_package.html
#]=======================================================================]

find_library(PAPI NAMES papi)

find_path(PAPI_INCLUDE papi.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PAPI
  DEFAULT_MSG PAPI PAPI_INCLUDE)

if(${PAPI_FOUND} AND NOT TARGET papi::papi)
  add_library(papi::papi UNKNOWN IMPORTED GLOBAL)
  set_target_properties(papi::papi PROPERTIES
    IMPORTED_LOCATION ${PAPI} INTERFACE_INCLUDE_DIRECTORIES ${PAPI_INCLUDE})
endif()
