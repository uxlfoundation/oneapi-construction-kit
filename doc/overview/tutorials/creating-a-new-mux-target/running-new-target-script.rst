Running the create_target.py Script
===================================

This part of the tutorial assumes a working *HAL* for the *RefSi* target. This
should be based off `hal_refsi_tutorial`.

The *RefSi* target is a RISC-V target which uses a command processor to run a
kernel with any given parameters across different instances and slices. These
parameters will all be identical for each kernel execution.

First of all we need an empty directory ``refsi_tutorial``. We also assume that
the ``oneAPI Construction Kit`` exists at ``$ONEAPI_CON_KIT_PATH`` commands:

.. code:: bash

  export ONEAPI_CON_KIT_PATH=<path_to_construction_kit>
  mkdir refsi_tutorial
  cd refsi_tutorial

We have provided a script which allows creating the bones of a new target. This
requires only to write a JSON file which describes the main parts of the target. 

Create a ``refsi.json`` file like this:

.. code:: json

   {
     "target_name": "refsi_tutorial",
     "llvm_name": "RISCV",
     "llvm_cpu": "\"generic-rv64\"",
     "llvm_features": "\"+m,+f,+a,+d,+c,+v,,+zbc,+zvl512b\"",
     "llvm_triple": "\"riscv64-unknown-elf\"",
     "vlen" : "512",
     "link" : "true",
     "scalable_vector": "true"
   }

This can also be found under ``$ONEAPI_CON_KIT_PATH/scripts/new_target_templates/refsi.json``.

The `JSON` attribute-value pairs are explained in the table below: 

.. list-table:: target description
   :widths: 25 25
   :header-rows: 1

   * - Entry
     - Description
   * - "target_name": "refsi_tutorial" 
     - This is used for the `mux` target API naming.
   * - "llvm_name": "RISCV"
     - Used for llvm initializaton e.g. LLVMInitializeRISCVTarget()
   * - "llvm_cpu": \""generic-rv64\""
     - The cpu description used for LLVM
   * - "llvm_features": \""+m,+f,+a,+d,+c,+v\""
     - The features description used for LLVM, done as a a comma seperated
       list. In this case we are enabling all the extensions used in refsi.
   * - "llvm_triple": ""riscv64-unknown-elf" 
     - The llvm target triple used for refsi.
   * - "vlen": "512" 
     - The vector length, used for scalable calculations.
   * - "link" : "true"
     - Whether we wish to call the linker.
   * - "scalable": "true"
     - This means we support scalable vectors (from the RISC-V rvv extension).
   * - "command_line_options": "\"--riscv-v-vector-bits-min=512\""
     - This is options to be passed through to the llvm backend that need to be
       set. In this case we are telling the backend that vlen is 512.
 
Now we will run the script inside the ``oneAPI Construction Kit`` directory:

.. code:: console

    $ cd refsi_tutorial
    $ $ONEAPI_CON_KIT_PATH/scripts/create_target.py $ONEAPI_CON_KIT_PATH \
       $ONEAPI_CON_KIT_PATH/scripts/new_target_templates/refsi.json \
       --external-dir $PWD

The first parameter is the path to the ``oneAPI Construction Kit``. The second
parameter is the `json` file discussed previously. The third parameter is the
external directory which `oneAPI Construction Kit` will require for building the
new target.

This creates subdirectories ``mux/refsi_tutorial`` and ``compiler/refsi_tutorial``,
named after the `target_name` field. It also creates a ``CMakeLists.txt`` which
can be used to build the oneAPI Construction Kit. After creating these new
directories, we have a fully buildable target, ready for your ``HAL``. The ``mux``
side handles the runtime aspects and the default generated here assumes it is on the
same architecture as the host we build on. The ``compiler`` side manages the
compilation of kernels. It has a standard LLVM pipeline influenced by the `json`
file, which is used to produce executable kernels.

The ``HAL`` should come from the first tutorial, although the repo has a branch
`tutorial1_step5_sub5` which matches the final part of the tutorial.

The generated `CMakeLists.txt` is very simple and will look something like this:

.. code:: cmake

  project(refsi_tutorial)
  cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)

  set(CA_EXTERNAL_MUX_TARGET_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/mux/refsi_tutorial"
    CACHE STRING "override" FORCE)
  set(CA_EXTERNAL_MUX_COMPILER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/compiler/refsi_tutorial"
    CACHE STRING "override" FORCE)

  set(CA_EXTERNAL_REFSI_TUTORIAL_HAL_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/hal_refsi_tutorial" CACHE STRING "External oneAPI Construction Kit HAL")

  set(CA_EXTERNAL_ONEAPI_CON_KIT_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/ONEAPI_KIT" CACHE STRING "External oneAPI Construction Kit")

  add_subdirectory(${CA_EXTERNAL_ONEAPI_CON_KIT_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/oneAPIConstructionKit)

The ``CA_EXTERNAL_MUX_TARGET_DIRS`` and ``CA_EXTERNAL_MUX_COMPILER_DIRS`` are
used to tell the oneAPI Construction Kit where to look for the per target code,
both for ``mux`` (the runtime) and ``compiler`` (the code generation). The
directory name should match the target name.

``CA_EXTERNAL_REFSI_TUTORIAL_HAL_DIR`` indicates where to look for the `HAL`
target. This can be changed to wherever you have stored the final
`hal_refsi_tutorial`, but defaults to within the current top level directory.

``CA_EXTERNAL_ONEAPI_KIT_DIR`` is used to indicate where the `oneAPI Construction Kit` directory is.


Both of these variables can be overridden on the `cmake` line.
