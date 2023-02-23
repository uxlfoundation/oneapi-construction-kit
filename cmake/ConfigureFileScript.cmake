# Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
