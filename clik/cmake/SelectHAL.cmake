# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Fetch the list of known HALs.
get_property(HAL_NAMES GLOBAL PROPERTY KNOWN_HAL_DEVICES)
foreach(HAL_NAME ${HAL_NAMES})
  message(STATUS "Found HAL: ${HAL_NAME}")
endforeach()

# Let the user choose the HAL using a list (in the GUI)
set(CLIK_HAL_NAME "cpu" CACHE STRING "Name of the HAL library to use by default")
set_property(CACHE CLIK_HAL_NAME PROPERTY STRINGS ${HAL_NAMES})

# Create a function that compiles a kernel for the selected HAL.
# TODO: Use cmake_language (https://cmake.org/cmake/help/v3.18/command/cmake_language.html)
if(NOT CLIK_HAL_NAME STREQUAL "")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/compile_kernel.cmake "
function(hal_compile_kernel_source)
  hal_${CLIK_HAL_NAME}_compile_kernel_source(\${ARGN})
endfunction()

function(hal_link_kernel)
  hal_${CLIK_HAL_NAME}_link_kernel(\${ARGN})
endfunction()
"
  )
  include("${CMAKE_CURRENT_BINARY_DIR}/compile_kernel.cmake")
endif()
