# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_subdirectory(loader)
add_subdirectory(common)

add_library(hal_refsi SHARED
  hal_main.cpp
  refsi_hal.cpp
  refsi_hal_m1.cpp
  refsi_hal_g1.cpp
  refsi_command_buffer.cpp
  riscv_encoder.cpp
)

target_compile_definitions(hal_refsi PUBLIC -DBUILD_HAL_DLL)
target_compile_definitions(hal_refsi PUBLIC -DNUM_HARTS_FOR_CA_MODE=${NUM_HARTS_FOR_CA_MODE})
if(HAL_REFSI_SOC STREQUAL "M1")
  target_compile_definitions(hal_refsi PUBLIC -DHAL_REFSI_TARGET_M1 -DHAL_REFSI_MODE_WG=1)
elseif(HAL_REFSI_SOC STREQUAL "G1")
  target_compile_definitions(hal_refsi PUBLIC -DHAL_REFSI_TARGET_G1)
  if (${HAL_REFSI_THREAD_MODE} STREQUAL "WG")
    target_compile_definitions(hal_refsi PUBLIC -DHAL_REFSI_MODE_WG=1)
  else()
    target_compile_definitions(hal_refsi PUBLIC -DHAL_REFSI_MODE_WI=1)
  endif()
endif()

target_include_directories(hal_refsi PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/loader
)

target_include_directories(hal_refsi PUBLIC
    ${HAL_REFSI_SOURCE_DIR}/include
)

target_link_libraries(hal_refsi hal_common dl refsi_common refsidrv)

add_dependencies(hal_refsi refsi_g1_loader_binary)

add_baked_data(hal_refsi
    hal_refsi_linker_script
    linker_script.h
    ${HAL_REFSI_SOURCE_DIR}/include/device/program.lds)

install(TARGETS hal_refsi
        LIBRARY DESTINATION lib
)

set_target_properties(hal_refsi PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
