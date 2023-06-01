RISC-V
======

Codeplay's reference RISC-V target, for RISC-V implementation. The intention of
this target is to provide a flexible way of communicating with a variety of
customer RISC-V targets, with different RISC-V configurations. This target
supports multiple different variants by using an abstract class (HAL), which is
used to configure the target and act on commands such as enqueuing kernels and
allocating and reading or writing to memory. The current version has only been
tested with an x86_64 host CPU.

The current in-tree targets are variants of Codeplay's reference
architecture(RefSi). This comes in two variants `G` and `M1`. The `riscv` target
matches `G` and has no need for anything architecture specific except for what is
needed to support `riscv`. `M1` has additional hardware features, such as DMA.

The ``riscv`` target uses a common utility ``riscv`` compiler library which can be
used or derived from for different targets. 

The RISC-V target can also be built with just the compiler aspect changed. This
is shown with `M1` under ``examples/refsi/refsi_m1`` of the oneAPI Construction Kit.

HAL
---

The HAL is an abstract class which is required to be used with this target. This
abstract class will be accessed through a shared library which will be opened at
runtime. This is done in ``hal.cpp``, ``riscv::hal_get()``, where it uses
dynamic loading. This provides a ``hal_t`` class. From this ``hal_t`` class, we
can get information about the general HAL target (``hal_info_t``), more detailed
information about the devices (``hal_device_info_t``) and create or free a HAL
device (``hal_device_t``).

.. seealso::
   For more detailed information on the HAL, see the :doc:`specification
   </specifications/hal>` and `:doc:`dynamic loading
   </modules/mux/hal/dynamic_loading>`.

From the target's viewpoint we mostly interact with ``hal_device_info_t`` for
information and ``hal_device_t`` for actions. ``hal_device_t`` gives us the
following:

1. A method to allocate memory and read and write to memory
2. A method to load a kernel (as an ELF file)
3. A method to enqueue a kernel across a range and a set of arguments.

All methods are currently seen as blocking (or effectively blocking). The HAL
does not specify anything about the contents of the ELF file in itself, but the
current compilation makes assumptions that the ELF file will have a certain
interface to the arguments for each kernel function see
`RISC-V standard function arguments`_.

``hal_device_info_t``
 Is a base class which is not RISC-V specific, and there is a RISC-V specific
 one which is derived from this. The HAL is used in the following way:

1. ``hal_device_info_riscv_t`` provides information about the type of RISC-V
processor. This includes extensions and ABI information. This information is
used in ``modules/mux/targets/riscv/source/kernel.cpp`` to help it build a linked
ELF file for this particular processor configuration. At no other point does any
of the target reference anything specific about RISC-V.

2. ``hal_device_info_t`` is expected to give a lot of information which can be
used to populate oneAPI Construction Kit's device information. This is not specific to
RISC-V. This includes information such as global memory size, address size etc.

3. ``specialized kernel`` is used to process the incoming arguments and create a
list of HAL arguments for enqueuing a kernel.

4. In ``queue.cpp``, enqueuing of a kernel across a range is done by using the
previously created list of HAL arguments, loading the kernel and calling the HAL
enqueue of NDRange.

5. In ``queue.cpp``, reading, writing and filling of buffers happen by calling the
equivalent method on the HAL.

6. In ``memory.cpp``, we support reading and writing of memory by calling the
equivalent function on the HAL.

The HAL is versioned with respect to any API changes, so if something changes in
the interface the version must too. The version in ``hal.cpp`` of
``expected_hal_version`` must match the HAL device. This may be as simple as a
recompile depending on the change.

RISC-V standard function arguments
----------------------------------

The generated linked ELF file is expected to contain functions that have a
defined format. For each kernel we have:

.. code-block:: cpp

  <function_name>(void *argsStruct, WorkGroupInfo *)

``argsStruct`` is actually a block of memory which represents all of the
arguments. Each one is placed in order into the memory and is aligned to a power
of 2 greater than or equal to the size of the argument. For example if we have a
``short``, followed by a ``uint``, the ``uint`` would be 4 byte aligned and
start at 4 byte aligned offset and ``short`` would be aligned to 2 bytes.
``short8`` would be aligned to 16 bytes.

The second argument tells us about the current workgroup that is to be acted on.
The kernel function should work on the whole workgroup for each call to the
kernel function.

.. seealso::
  For more information on this struct see the documentation in the HAL
  repository of oneAPI Construction Kit.

The standard RISC-V ABI is used currently, regardless of any HAL choices.

RISC-V Device
-------------

The information reported by a RISC-V device can vary depending on the build
configuration of oneAPI Construction Kit. See the
:ref:`developer-guide:Cmake Options` for details on the effects of RISC-V
specific CMake options.

Build Options
#############

Currently recommended build options include:

.. code-block:: console

 $ cmake -GNinja \
   -DCA_RISCV_ENABLED=ON \
   -DCA_MUX_TARGETS_TO_ENABLE="riscv" \
   -DCA_LLVM_INSTALL_DIR=<llvm_install_dir>/llvm_install \
   -DCA_ENABLE_HOST_IMAGE_SUPPORT=OFF \
   -DCA_CL_ENABLE_ICD_LOADER=ON ..

This will build a 'G' compatible version. To build a 'M' compatible version we
can keep the same ``mux`` target, but use a different compiler target as the 'M'
target has additional features. This is done by adding to the build options:

.. code-block:: console

 $ cmake -GNinja \
   -DCA_RISCV_ENABLED=ON \
   -DCA_MUX_TARGETS_TO_ENABLE="riscv" \
   -DCA_LLVM_INSTALL_DIR=<llvm_install_dir>/llvm_install \
   -DCA_ENABLE_HOST_IMAGE_SUPPORT=OFF \
   -DCA_CL_ENABLE_ICD_LOADER=ON
   -DCA_EXTERNAL_MUX_COMPILER_DIRS=<ddk_dir>/examples/refsi/refsi_m1/compiler/refsi_m1
   -DCA_MUX_COMPILERS_TO_ENABLE="refsi_m1" ..

``CA_EXTERNAL_MUX_COMPILER_DIRS`` tells us to also use an additional compiler
directory. ``CA_MUX_COMPILERS_TO_ENABLE`` tells us to only enable this compiler
directory; this is needed to stop it also building the `riscv` target as well and
both being attached to the ``mux`` target.


The default HAL is ``hal_refsi`` and it looks for it in
``examples/refsi/hal_refsi``. However if a directory
``CA_RISCV_EXTERNAL_HAL_DIR`` is given it will look there. This will currently
also require ``CA_HAL_NAME`` to be set if the name differs from the default.

.. note::
  The installed LLVM must have RISCV as an enabled target and build ``lld`` with
  ``-DLLVM_ENABLE_PROJECTS='clang;lld'``.

The following build options can also be useful:

``CA_HAL_NAME``
  Defines the default HAL which should be linked in. This will be used to link
  with the shared library, which should be of name ``libhal_<CA_HAL_NAME>.so.``

``HAL_DESCRIPTION``
  Is used to help the Mux target set up aspects which have to be done at build
  time. It can also be picked up by the HAL being built to configure the HAL if
  needed. These aspects include the 32/64 bit capabilities and floating point
  and double support. This is largely needed to create the
  :doc:`abacus builtins<builtins/abacus>`. This string should match the RISC-V
  string which it is related to.

``CA_ENABLE_HOST_IMAGE_SUPPORT``
  Disabled due to not supporting images but some prebuilt kernels not checking
  the support.

``CA_HAL_LOCK_DEVICE_NAME``
  Is a bool (defaulted to true), which can be used to allow loading of a
  different HAL to the default at runtime, as described in the dynamic loading
  documentation in the oneAPI Construction Kit HAL repository.


``CA_RISCV_DEMO_MODE``
  Is a bool (defaulted to false), which can be used to set environment variables
  for debug purposes to demonstrate the execution of a kernel on RISC-V. Note
  for a `Refsi M1` example build this will be CA_RISCV_M1_DEMO_MODE.

.. note::
  ICD support is optional.

Environment Variables
---------------------

The following environment variables are currently supported:

``CA_RISCV_VF``
  Used for setting the vectorization factor - see `Compilation`_.

``CA_HAL_DEVICE``
  Allows overriding of the HAL to be used at runtime. Only
  supported if built with ``-DCA_HAL_LOCK_DEVICE_NAME=OFF`` - see
  the dynamic loading documentation in the oneAPI Construction Kit HAL repository for more
  information.

``CA_RISCV_EARLY_LINK_BUILTINS``
  Link builtins before the vectorizer is run if set to 1. This is particularly
  important for use with scalable vectorization for which the builtins do not
  create scalable vector equivalents. When scalable vectorization is enabled
  this will default to true, otherwise false.

``CA_RISCV_DUMP_IR```
  Used to dump the generated IR at the beginning of the "late target passes"
  stage to stdout. Demo mode or debug mode only.

Additionally the following may be used by HALs to override their local setting,
although this is not mandatory.

``CA_RISCV_VLEN_BITS_MIN``
  Sets the minimum reported minimum ``VLEN`` bits - see `Compilation`_. This may
  override the VLEN if a HAL supports it. This should only be used if the actual VLEN
  used in the device is updated.

``CA_RISCV_SAVE_ELF_PATH``
  Path to elf file for dumping built executable. Demo mode or debug mode only.

``CA_RISCV_DUMP_ASM``
  If defined, output final assembly produced to stdout. Demo mode or debug mode only.

RISC-V Binaries
---------------

RISC-V can generate and accept binary executables, possibly containing multiple
kernels each. They use ELF files generated from LLVM. Both binaries and
compilation of source is managed in ``executable.cpp``. The contents of the
produced binaries are used in the various kernel classes, before finally being
loaded to the HAL in ``queue.cpp``.

Executable
----------

``riscvCreateExecutable()`` is used to either compile a bitcode file or use a
previously built binary to generate an executable. Builtin kernels are not
currently supported. For both cases we create a
``riscv::binary_executable_data_s`` which is used to contain the ELF data in a
dynamic array. This is created as a shared pointer so it can be passed through
the various kernel types, rather than copying the data multiple times, as the
executable could be deleted before the kernels are.

If it is given bitcode, it passes to an upcasted riscv version of the
``finalizer`` object, and calls ``createBinaryFromSource()`` directly on it,
which is explained in more detail in `Compilation`_.

Kernel Objects
--------------

``riscv::kernel_s``
  The first stage of the kernel objects and just contains the shared executable
  and the kernel name.

``riscv::scheduled_kernel_s``
  The next stage and contains the local size as well as the shared executable.

``riscv::specialized_kernel_s``
  The final stage and it is here that the global size as well as the kernel
  arguments are brought in. In ``riscvCreateSpecializedKernel()``, we process
  the descriptors passed in as parameters. These descriptors give information
  about each argument. These largely map one to one for each argument to
  equivalent ``hal::hal_arg_t``. In this function we create a vector of
  ``hal_arg_t`` objects and pass it to the created
  ``riscv::specialized_kernel_s``. This object also contains the global size of
  ``hal_arg_t`` values can be created. This specialized kernel is later pushed
  onto the command queue in ``riscvPushNDRange()`` and processed in
  ``threadPoolProcessCommands()`` in ``queue.cpp``.

Compilation
-----------

All actual compilation is done in the ``finalizer`` class method
``createBinaryFromSource()``. The first thing we do is upcast the
``hal_device_info_t`` and find out what extensions are supported in order to
initialize the target machine. We then read in the bitcode and turn it into an
LLVM Module. At this point we can run all the passes.

We also set ``--riscv-v-vector-bits-min`` based on the hal_device_info_t value
vlen if it exists and is non-zero, and enable :doc:`vecz` if ``CA_RISCV_VF`` is
set (or vector flags are enabled at the OpenCL options level).

``CA_RISCV_VF`` is defined as a comma separated list as follows:

* **S** - Use scalable vectorization
* **V** -  Vectorize only, otherwise produce both scalar and vector kernels
* **A** - Let Vecz automatically choose the vectorization factor
* **1-64** - Vectorization factor multiplier: the fixed amount itself, or the
  value that multiplies the scalable amount

.. note::
  For example, ``CA_RISCV_VF=4`` or ``CA_RISCV_VF=S,1``

All but one of the passes are util or LLVM passes. The util ones are detailed
:doc:`/modules/compiler/utils`, but the basics are as follows:

* :ref:`compiler::utils::AlignModuleStructsPass
  <modules/compiler/utils:AlignModuleStructsPass>`

* ``riscv::IRToBuiltinReplacementPass`` -
  A bespoke pass to handle some IR which currently produces link errors. This
  currently only includes ``frem`` and converts it a call to the ``fmod``
  builtin which is then handled by the :doc:`abacus builtins<builtins/abacus>`.

* :ref:`vecz::RunVeczPass<modules/compiler/utils:RunVeczPass>`

* :ref:`compiler::utils::LinkBuiltinsPass<modules/compiler/utils:LinkBuiltinsPass>`

* :ref:`compiler::utils::ReplaceMuxMathDeclsPass<modules/compiler/utils:ReplaceMuxMathDeclsPass>`

* ``llvm::InternalizePass`` - Used to help remove dead barrier calls after
  inlining

* :ref:`compiler::utils::FixupCallingConventionPass <modules/compiler/utils:FixupCallingConventionPass>`

* :ref:`compiler::utils::HandleBarriersPass <modules/compiler/utils:HandleBarriersPass>`

* :ref:`compiler::utils::AddSchedulingParametersPass <modules/compiler/utils:AddSchedulingParametersPass>`

* :ref:`compiler::utils::DefineMuxBuiltinsPass <modules/compiler/utils:DefineMuxBuiltinsPass>`

* :ref:`compiler::utils::AddKernelWrapperPass
  <modules/compiler/utils:AddKernelWrapperPass>` - Note that the use of this
  does not pack the args, but uses alignment to the power of 2 equal to or
  above the size of each argument

* :ref:`compiler::utils::ReplaceLocalModuleScopeVariablesPass
  <modules/compiler/utils:ReplaceLocalModuleScopeVariablesPass>`

After running these passes all kernels should have the appropriate function
signature of the argument structure and the schedule struct.

We then emit to a file and call LLD to link the final object. The
``hal_device_info_t`` gives the linker script to use. At this point we have an
ELF file which will be untouched until it gets passed to the HAL to load.

Processing commands
-------------------

``riscv::command_group_s`` is used to maintain a vector of commands which are
later processed in ``queue.cpp``. This is identical to the :doc:`host`
code, except it does not support images and ``host`` is renamed to ``riscv``.

The riscv device maintains a threadpool. This is more complicated than it needs
to be for our needs. Its main role here is to process the queued command and
signal semaphores as needed when operations are done.

The main function of interest is ``threadPoolProcessCommands()``. This acts on
the command from the queue. This command can be one of the following:

* ``command_type_read_buffer``
* ``command_type_write_buffer``
* ``command_type_fill_buffer``
* ``command_type_copy_buffer`` - read, write, fill and copy map directly onto
  ``hal_device_t`` equivalents
* ``command_type_user_callback``
* ``command_type_begin_query``
* ``command_type_end_query``
* ``command_type_reset_query_pool`` - These do not touch the HAL and use the
  query pool code in ``query_pool.cpp``, which is very similar to that of
  ``host`` target.
* ``command_type_ndrange`` - calls ``exec_command_type_ndrange()``, see below.

``exec_command_type_ndrange()`` uses multiple ``hal_device_t`` methods. It does
the following:

1. Loads the ELF file from the specialized kernel onto the device using
   ``hal_device->program_load()``.

2. It finds the entry point of the kernel, using
   ``hal_device->program_find_kernel()``

3. It executes the kernel across the ndrange using
   ``hal_device->kernel_exec()``.
