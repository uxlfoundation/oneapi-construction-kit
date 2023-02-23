# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_library(kts STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/arguments_shared.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/execution_shared.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/reference_functions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/stdout_capture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/vecz_tasks_common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/arguments_shared.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdout_capture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/reference_functions_shared.cpp)

target_compile_options(kts PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:
    -wd4996  # Disable warning on deprecated declarations
  >)

target_include_directories(kts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kts PUBLIC ca_gtest)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES kts)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")
