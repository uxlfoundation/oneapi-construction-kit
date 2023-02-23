# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(VK_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VK_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/UnitVK)

add_ca_check_group(vk DEPENDS check-spirv-ll-lit check-UnitVK)
