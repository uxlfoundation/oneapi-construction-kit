Kernel Compilation
------------------

In the previous part, we have built clik (an OpenCL-like runtime library) and
the skeleton RefSi HAL (which is responsible for interfacing with the RefSi
'hardware'). The last component we need to build in order to develop the RefSi
HAL is the clik test suite, made up of several examples that use clik to offload
simple kernels to a device. At this stage, trying to build the clik examples
will fail:

.. code:: console

    $ ninja ClikExamples
     ...
      CMake Error at path/to/refsi_tutorial_part1/clik/cmake/Bin2HScript.cmake:40 (message):
      File 'path/to/refsi_tutorial_part1/build/examples/clik_async/hello/hello_async_kernel' does not exist!

      [1/38] Generating H file examples/clik_async/vector_add/kernel_binary.h
      FAILED: examples/clik_async/vector_add/kernel_binary.h

Such errors occur because building clik examples involves compiling each kernel
source file to a binary, generating a C header file from the binary
(``kernel_binary.h`` for the above example) and including this header in the
example that uses the kernel. Since kernels are not yet compiled to binaries,
the step which generates C headers from these binaries fails.

This kernel compilation approach, where kernels are compiled at the same time as
the application that uses them, is called 'offline compilation'. In contrast,
the compilation approach usually chosen for OpenCL applications is an 'online
compilation' approach, where kernels are compiled when the application runs. In
the online approach, the runtime library (e.g. OpenCL or clik) must also include
a compiler (typically, LLVM) that is able to compile kernels at run-time.

An offline approach was chosen for clik, which allows using an off-the-shelf
compiler such as GCC or Clang for compiling kernels (which are written in C) and
keeping the runtime library (as well as build times) as small as possible. In
this tutorial, we are using the 64-bit RISC-V variant of the GCC compiler which
is part of the Ubuntu 22.04 OS for compiling clik kernels to RISC-V binaries.

In the RefSi skeleton HAL, the step that is responsible for compiling kernels
(the ``hal_refsi_tutorial_compile_kernel_source`` and
``hal_refsi_tutorial_link_kernel`` functions in
``refsi_hal/cmake/CompileKernel.cmake``) have been left to be completed:


.. code:: cmake

    function(hal_refsi_tutorial_compile_kernel_source OBJECT SRC)
      set(INCLUDES ${ARGN})
      get_property(RISCV_CC_FLAGS GLOBAL PROPERTY RISCV_CC_FLAGS)
      get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
      get_property(ROOT_DIR GLOBAL PROPERTY HAL_REFSI_TUTORIAL_DIR)
      get_target_property(REFSIDRV_SRC_DIR refsidrv SOURCE_DIR)

      # TODO: Compile a kernel source file (${SRC}) into a kernel object (${OBJECT})
    endfunction()
    
    function(hal_refsi_tutorial_link_kernel BINARY)
      get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
      get_property(ROOT_DIR GLOBAL PROPERTY HAL_REFSI_TUTORIAL_DIR)
      
      # TODO: Link multiple objects (${OBJECTS}) into a kernel executable (${BINARY})
    endfunction()

The first function can be completed by replacing the ``# TODO: ...`` comment
with the following:

.. code:: cmake

    set(LINKER_SCRIPT ${ROOT_DIR}/include/device/program.lds)
    set(KERNEL_CFLAGS -DBUILD_FOR_DEVICE)
    list(APPEND INCLUDES ${REFSIDRV_SRC_DIR}/../include/device)
    list(APPEND INCLUDES ${ROOT_DIR}/include/device)
    foreach(INCLUDE ${INCLUDES})
      set(KERNEL_CFLAGS ${KERNEL_CFLAGS} -I${INCLUDE})
    endforeach()
    add_custom_command(OUTPUT ${OBJECT}
                       COMMAND ${RISCV_CC} ${RISCV_CC_FLAGS} -O2 -c -nodefaultlibs -fno-stack-protector ${KERNEL_CFLAGS} ${SRC} -o ${OBJECT}
                       DEPENDS ${SRC})

The above ``add_custom_command`` command invokes the system RISC-V C compiler
(``${RISCV_CC}``) to compile the source file (``${SRC}``) to an object file
(``${OBJECT}``). Include paths that contains headers required by the kernel are
also passed to the compiler. The various other flag variables
(``${RISCV_CC_FLAGS}`` and ``${KERNEL_CFLAGS}``) are also necessary for
compiling kernels but are not as important when it comes to understanding the
kernel compilation process.

The second function can be completed by replacing the ``# TODO: ...`` comment
with the following, which invokes the linker to create an executable from the
objects produced from calling ``hal_refsi_tutorial_compile_kernel_source`` on
each kernel source file:

.. code:: cmake

    set(LINKER_SCRIPT ${ROOT_DIR}/include/device/program.lds)
    add_custom_command(OUTPUT ${BINARY}
                       COMMAND ${RISCV_CC} ${RISCV_CC_FLAGS} -static ${RISCV_LINKER_FLAGS} -nodefaultlibs ${OBJECTS} -Wl,-e -Wl,kernel_main -Wl,--build-id=none -o ${BINARY} -T${LINKER_SCRIPT}
                       DEPENDS ${OBJECTS} ${LINKER_SCRIPT})

Note how a linker script (``${LINKER_SCRIPT}``) is needed to lay out the binary
executable in a way that can be loaded on the RefSi device.

Once these changes are made to ``CompileKernel.cmake``, it is now possible to build
all the clik examples without error:

.. code:: console

    $ ninja ClikExamples
      [7/50] Generating blur_kernel
      /usr/lib/gcc-cross/riscv64-linux-gnu/9/../../../../riscv64-linux-gnu/bin/ld: warning: cannot find entry symbol kernel_main; defaulting to 0000000000010000
      [50/50] Linking CXX executable bin/matrix_multiply_tiled

