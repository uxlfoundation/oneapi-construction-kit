# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_subdirectory(common)

add_library(hal_refsi_tutorial SHARED
  hal_main.cpp
  refsi_hal.cpp
)

target_compile_definitions(hal_refsi_tutorial PUBLIC -DBUILD_HAL_DLL)

target_include_directories(hal_refsi_tutorial PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(hal_refsi_tutorial PUBLIC
    ${HAL_REFSI_TUTORIAL_SOURCE_DIR}/include
)

target_link_libraries(hal_refsi_tutorial hal_common dl refsi_common refsidrv)

add_baked_data(hal_refsi_tutorial
    hal_refsi_tutorial_linker_script
    linker_script.h
    ${HAL_REFSI_TUTORIAL_SOURCE_DIR}/include/device/program.lds)

install(TARGETS hal_refsi_tutorial
        LIBRARY DESTINATION lib
)

set_target_properties(hal_refsi_tutorial PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
