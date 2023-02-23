# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_library(clik_runtime_sync SHARED
  clik_sync.cpp
)

target_link_libraries(clik_runtime_sync hal_common)

target_include_directories(clik_runtime_sync PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(clik_runtime_sync PUBLIC -DCLIK_DLL)
target_compile_definitions(clik_runtime_sync PRIVATE -DCLIK_HAL_NAME="${CLIK_HAL_NAME}")

install(TARGETS clik_runtime_sync
        LIBRARY DESTINATION lib
)

install(FILES ${PROJECT_SOURCE_DIR}/include/clik_sync_api.h DESTINATION include)
