# Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
