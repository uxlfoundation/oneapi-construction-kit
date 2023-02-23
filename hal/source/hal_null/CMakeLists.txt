# Copyright (C) Codeplay Software Limited. All Rights Reserved.

include(GNUInstallDirs) # Needed for CMAKE_INSTALL_LIBDIR

add_library(hal_null SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_null.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/hal_null.cpp)

target_include_directories(hal_null PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(hal_null PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

install(
  TARGETS hal_null
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_link_libraries(hal_null PUBLIC hal_common)
target_compile_options(hal_null PRIVATE -DBUILD_HAL_DLL)
