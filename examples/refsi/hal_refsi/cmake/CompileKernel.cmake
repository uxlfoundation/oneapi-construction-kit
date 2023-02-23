# Copyright (C) Codeplay Software Limited. All Rights Reserved.

function(hal_refsi_compile_kernel_source OBJECT SRC)
  set(INCLUDES ${ARGN})
  get_property(RISCV_CC_FLAGS GLOBAL PROPERTY RISCV_CC_FLAGS)
  get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
  get_property(ROOT_DIR GLOBAL PROPERTY HAL_REFSI_DIR)
  get_target_property(REFSIDRV_SRC_DIR refsidrv SOURCE_DIR)
  set(DRIVER_INCLUDE_DIR ${REFSIDRV_SRC_DIR}/../include/device)
  set(DEVICE_INCLUDE_DIR ${ROOT_DIR}/include/device)
  set(LINKER_SCRIPT ${ROOT_DIR}/include/device/program.lds)
  set(KERNEL_CFLAGS -DBUILD_FOR_DEVICE)
  list(APPEND INCLUDES ${DEVICE_INCLUDE_DIR})
  list(APPEND INCLUDES ${DRIVER_INCLUDE_DIR})
  foreach(INCLUDE ${INCLUDES})
    set(KERNEL_CFLAGS ${KERNEL_CFLAGS} -I${INCLUDE})
  endforeach()
  if(HAL_REFSI_SOC STREQUAL "M1")
    set(KERNEL_CFLAGS ${KERNEL_CFLAGS} -DHAL_REFSI_TARGET_M1=1)
  elseif(HAL_REFSI_SOC STREQUAL "G1")
    set(KERNEL_CFLAGS ${KERNEL_CFLAGS} -DHAL_REFSI_TARGET_G1=1)
  endif()

  add_custom_command(OUTPUT ${OBJECT}
                     COMMAND ${RISCV_CC} ${RISCV_CC_FLAGS} -O2 -c ${RISCV_LINKER_FLAGS} -nodefaultlibs -fno-stack-protector ${KERNEL_CFLAGS} ${SRC} -o ${OBJECT}
                     DEPENDS ${SRC})
endfunction()

function(hal_refsi_link_kernel BINARY)
  set(OBJECTS ${ARGN})
  get_property(EXTRA_OBJECTS GLOBAL PROPERTY HAL_REFSI_EXTRA_OBJECTS)
  foreach(OBJECT ${EXTRA_OBJECTS})
    list(APPEND OBJECTS ${OBJECT})
  endforeach()

  get_property(RISCV_CC_FLAGS GLOBAL PROPERTY RISCV_CC_FLAGS)
  get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
  get_property(ROOT_DIR GLOBAL PROPERTY HAL_REFSI_DIR)
  set(LINKER_SCRIPT ${ROOT_DIR}/include/device/program.lds)

  add_custom_command(OUTPUT ${BINARY}
                     COMMAND ${RISCV_CC} ${RISCV_CC_FLAGS} -static ${RISCV_LINKER_FLAGS} -nodefaultlibs ${OBJECTS} -Wl,-e -Wl,kernel_main -Wl,--build-id=none -o ${BINARY} -T${LINKER_SCRIPT}
                     DEPENDS ${OBJECTS} ${LINKER_SCRIPT})
endfunction()
