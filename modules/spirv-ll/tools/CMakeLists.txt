# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_executable(spirv-ll-tool ${CMAKE_CURRENT_SOURCE_DIR}/spirv-ll.cpp)
target_link_libraries(spirv-ll-tool PRIVATE spirv-ll)
