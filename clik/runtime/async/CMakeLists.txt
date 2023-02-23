# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_library(clik_runtime_async SHARED
  clik_async.cpp
  clik_command_queue.cpp
)

target_link_libraries(clik_runtime_async hal_common pthread)

target_include_directories(clik_runtime_async PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(clik_runtime_async PUBLIC -DCLIK_DLL)
target_compile_definitions(clik_runtime_async PRIVATE -DCLIK_HAL_NAME="${CLIK_HAL_NAME}")

install(TARGETS clik_runtime_async
        LIBRARY DESTINATION lib
)

install(FILES ${PROJECT_SOURCE_DIR}/include/clik_async_api.h DESTINATION include)
