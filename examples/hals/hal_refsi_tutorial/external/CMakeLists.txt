# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if (NOT TARGET hal_common)
  set(HAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/hal")
  if (NOT EXISTS ${HAL_DIR}/include/hal.h)
    message(FATAL_ERROR "Could not find HAL API headers. Is there a 'hal' checkout at ${HAL_DIR} ?")
  endif()
  add_subdirectory(hal)
endif()

if (NOT TARGET refsidrv)
  add_subdirectory(refsidrv)
endif()
