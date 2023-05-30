Building and Running Tests
==========================

A compatible *LLVM* needs to be built for RISC-V, it also needs *LLD* built. See
:ref:`version of LLVM<overview/compiler/supported-llvm-versions:RISC-V>`. We
suggest building with:

.. code:: console

    $ cd <llvm repo dir>
    $ mkdir build
    $ cd build
    $ cmake -GNinja \
        -DLLVM_TARGETS_TO_BUILD="RISCV" \
        -DCMAKE_INSTALL_PREFIX=$LLVM_INSTALL_DIR \
        -DLLVM_ENABLE_PROJECTS='clang;lld' \
        -DLLVM_ENABLE_ASSERTIONS=On \
        -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_INSTALL_UTILS=ON \
        ../llvm
    $ ninja install

To build we first of all need to create a build directory inside your top level directory.
Firstly we set up the `CMake` directive.

.. code:: console

    $ mkdir build
    $ export LLVM_INSTALL_DIR=<your_llvm_install_dir>
    $ export ONEAPI_CON_KIT_PATH=<your_oneapi_construction_kit_dir>    
    $ cmake -GNinja -DCA_MUX_TARGETS_TO_ENABLE="refsi_tutorial" \
        -DCA_REFSI_TUTORIAL_ENABLED=ON -DCA_ENABLE_API=cl \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCA_LLVM_INSTALL_DIR=$LLVM_INSTALL_DIR \
        -DCA_ENABLE_DEBUG_SUPPORT=ON \
        -DCA_ENABLE_OFFLINE_LIBRARIES=OFF -DCA_ENABLE_HOST_IMAGE_SUPPORT=OFF \
        -DCA_CL_ENABLE_OFFLINE_KERNEL_TESTS=OFF \
        -DCA_EXTERNAL_ONEAPI_CON_KIT_DIR=$ONEAPI_CON_KIT_PATH \
        -DCA_EXTERNAL_REFSI_TUTORIAL_HAL_DIR=<path to target hal> \
        -Bbuild .

The ``CA_MUX_TARGETS_TO_ENABLE`` should match the ``target_name`` field from the
`JSON` file.

Now we build the ``UnitCL`` test target, which will also build all its
dependencies, including the oneAPI Construction Kit and the new target:

.. code:: console

    $ cd build/oneAPIConstructionKit
    $ ninja UnitCL

We can do a quick test, using the environment variable `REFSI_DEBUG` for more information:

.. code:: console

    $ REFSI_DEBUG=1 ./bin/UnitCL --gtest_filter=Execution/Execution.Task_01_02_Add/OpenCLC

You should see something similar to this:

.. code:: console

    Note: Google Test filter = Execution/Execution.Task_01_02_Add/OpenCLC
    [==========] Running 1 test from 1 test suite.
    [----------] Global test environment set-up.
    [----------] 1 test from Execution/Execution
    [ RUN      ] Execution/Execution.Task_01_02_Add/OpenCLC
    [CMP] Starting.
    [CMP] Starting to execute command buffer at 0x47fff1a0.
    [CMP] CMP_WRITE_REG64(ENTRY_PT_FN, 0x10000)
    [CMP] CMP_WRITE_REG64(KUB_DESC, 0x20000bff0f200)
    [CMP] CMP_WRITE_REG64(KARGS_INFO, 0x0)
    [CMP] CMP_WRITE_REG64(TSD_INFO, 0x1280000200000)
    [CMP] CMP_RUN_KERNEL_SLICE(n=4, slice_id=0, max_harts=0)
    [CMP] CMP_FINISH
    [CMP] Finished executing command buffer.
    ../modules/kts/include/kts/execution_shared.h:16: Failure
    Invalid data when validating buffer 2:
    Result mismatch at index 64 (expected: 512, actual: 0)
    [  FAILED  ] Execution/Execution.Task_01_02_Add/OpenCLC, where GetParam() = 0 (48 ms)

Although this runs, it actually returns a failure. This is because the compiler
pipeline is creating an executable for the kernel with a defined interface. This
interface does not match that expected for a kernel running in the ``RefSi``
architecture. We can resolve this by adding an additional pass to translate
between what ``RefSi`` kernels expect and what the default pipeline does. This is
shown in the next section.
