# Copyright (C) Codeplay Software Limited. All Rights Reserved.

#[=======================================================================[.rst:
A `Find Module`_ for detecting the Vulkan loader library, checking environment
variables set by the `Vulkan SDK`_ in addition to the default system locations.

.. seealso::
  Implemented using `FindPackageHandleStandardArgs`_

Variables
---------

This modules adds the following variables:

.. cmake:variable:: VULKAN_INCLUDE_DIR

  A filepath variable to the include directory containing ``vulkan/vulkan.h``.

.. cmake:variable:: VULKAN_LIBRARY

  A filepath variable to the directory containing the Vulkan loader libary.

.. cmake:variable:: VULKAN_FOUND

  A boolean CMake variable set to ``TRUE`` if both
  :cmake:variable:`VULKAN_LIBRARY` and :cmake:variable:`VULKAN_LIBRARY`
  are set and valid, ``FALSE`` otherwise.

Usage
-----

To use load this module either include it or use CMake `find_package`_.

.. code:: cmake

  include(FindVulkan)

.. code:: cmake

  find_package(Vulkan)

.. _Find Module:
  https://cmake.org/cmake/help/latest/manual/cmake-developer.7.html#find-modules
.. _FindPackageHandleStandardArgs:
  https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
.. _Vulkan SDK:
  https://www.lunarg.com/vulkan-sdk
.. _find_package:
  https://cmake.org/cmake/help/latest/command/find_package.html
#]=======================================================================]

if(WIN32)
  find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
    "$ENV{VULKAN_SDK}/Include"
    "$ENV{VK_SDK_PATH}/Include")
  if(CMAKE_CL_64)
    find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS
      "$ENV{VULKAN_SDK}/Bin" "$ENV{VK_SDK_PATH}/Bin")
    find_library(VULKAN_STATIC_LIBRARY NAMES vkstatic.1 HINTS
      "$ENV{VULKAN_SDK}/Bin" "$ENV{VK_SDK_PATH}/Bin")
  else()
    find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS
      "$ENV{VULKAN_SDK}/Bin32"
      "$ENV{VK_SDK_PATH}/Bin32")
  endif()
else()
  find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
    "$ENV{VULKAN_SDK}/include")
  find_library(VULKAN_LIBRARY NAMES vulkan HINTS
    "$ENV{VULKAN_SDK}/lib")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan
  DEFAULT_MSG VULKAN_LIBRARY VULKAN_INCLUDE_DIR)

mark_as_advanced(VULKAN_INCLUDE_DIR VULKAN_LIBRARY VULKAN_STATIC_LIBRARY)
