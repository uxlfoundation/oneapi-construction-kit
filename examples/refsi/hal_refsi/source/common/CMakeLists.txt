# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Compile common functions that are needed to execute kernels.
# Kernels will be linked against these objects.
set(IO_OBJ ${CMAKE_BINARY_DIR}/source/common/io.o)
hal_refsi_compile_kernel_source(${IO_OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/io.c)
add_custom_target(refsi_device_common DEPENDS ${IO_OBJ})
set_property(GLOBAL APPEND PROPERTY HAL_REFSI_EXTRA_OBJECTS ${IO_OBJ})
