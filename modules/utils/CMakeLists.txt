# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_library(utils STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/system.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/system.cpp)

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(utils PRIVATE
  $<$<BOOL:${CA_PLATFORM_LINUX}>:CA_PLATFORM_LINUX>
  $<$<BOOL:${CA_PLATFORM_WINDOWS}>:CA_PLATFORM_WINDOWS>
  $<$<BOOL:${CA_PLATFORM_MAC}>:CA_PLATFORM_MAC>
  $<$<BOOL:${CA_PLATFORM_ANDROID}>:CA_PLATFORM_ANDROID>
  $<$<BOOL:${CA_PLATFORM_QNX}>:CA_PLATFORM_QNX>)
target_link_libraries(utils PRIVATE cargo)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES utils)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")
