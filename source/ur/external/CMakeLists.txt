# Copyright (C) Codeplay Software Limited. All Rights Reserved.

include(FetchContent)

FetchContent_Declare(
  UNIFIED_RUNTIME
  GIT_REPOSITORY https://github.com/oneapi-src/unified-runtime.git
  GIT_TAG        6066cbb6a68feb377f2e031c706c9e123612d90a
)

# Fetch unified runtime from github. This also sets unified_runtime_SOURCE_DIR
FetchContent_Populate(UNIFIED_RUNTIME)  

add_library(unified-runtime-headers INTERFACE)
target_include_directories(unified-runtime-headers INTERFACE
  ${unified_runtime_SOURCE_DIR}/include)
