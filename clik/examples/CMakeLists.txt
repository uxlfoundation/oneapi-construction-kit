# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(CLIK_BUILD_MUX_EXAMPLES FALSE CACHE BOOL "Build Mux examples")

add_subdirectory(option_parser)

add_subdirectory(clik_sync)
add_subdirectory(clik_async)

if(CLIK_BUILD_MUX_EXAMPLES)
  add_subdirectory(mux)
endif()
