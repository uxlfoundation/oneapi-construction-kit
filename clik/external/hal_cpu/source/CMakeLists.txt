# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_library(hal_cpu SHARED
  hal_main.cpp
  cpu_hal.cpp
)

target_compile_definitions(hal_cpu PUBLIC -DBUILD_HAL_DLL)

target_include_directories(hal_cpu PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${HAL_CPU_SOURCE_DIR}/include
)

target_link_libraries(hal_cpu hal_common dl pthread)

hal_add_baked_data(hal_cpu
    hal_cpu_linker_script
    linker_script.h
    ${HAL_CPU_SOURCE_DIR}/include/device/program.lds)

install(TARGETS hal_cpu
        LIBRARY DESTINATION lib
)
