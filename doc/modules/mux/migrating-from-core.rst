Migrating a target from Core to ComputeMux
==========================================

This is a guide on how to migrate a target from the last version of the Core
specification to the ComputeMux Runtime and Compiler specifications.

It is not recommended to try and implement the new API as a shim on top of the
Core API, as there are certain fundamental changes that will add a significant
amount of complexity, however it's a good idea to have a copy of your Core
target in the source tree to refer back to during the migration.

Runtime
-------

The ComputeMux Runtime structure is very similar in structure to the Core
specification, and most of the API endpoints have simply been renamed. The
most significant change is the removal of the backend compilation steps, which
have been moved to the ComputeMux `Compiler`_.

To perform this migration, we suggest starting by copying your existing Core
implementation into a new directory for your Mux implementation, then going
through the ``Runtime`` sections in order. Note that in-tree Mux targets are in
``modules/mux/targets``, whilst in-tree Core targets used to be in
``modules/core/source``. ComputeAorta will no longer detect any targets
contained in ``modules/mux/source``.

CMake
~~~~~

For the most part, the Mux CMake API is the same as Core. The only difference
is that ``add_mux_target`` should be used instead of ``add_core_target``, and
any compilation related dependencies should be removed (such as LLVM).
Compilation related dependencies will be re-added later on in the Compiler
section.

Removals
~~~~~~~~

Functions around binaries and compilation from LLVM IR have been moved out of
the ComputeMux Runtime, so should be removed from your ComputeMux Runtime
implementation. These consist of:

* Binary objects and related functions

  * ``core_binary_t``
  * ``coreCreateBinaryFromSource``
  * ``coreCreateBinaryFromExecutable``
  * ``coreDestroyBinary``

* Finalizer objects and related functions have been removed, as executables can
  now only be created from binaries

  * ``core_finalizer_t``
  * ``coreCreateFinalizer``
  * ``coreDestroyFinalizer``

* Snapshot functions

  * ``coreListSnapshotStages``
  * ``coreSetSnapshotStage``

* Scheduled and specialized kernels and related functions

  * ``core_scheduled_kernel_t``
  * ``core_specialized_kernel_t``
  * ``coreCreateScheduledKernel``
  * ``coreDestroyScheduledKernel``
  * ``coreCreateSpecializedKernel``
  * ``coreDestroySpecializedKernel``

* Source type enums and execution options used by ``coreCreateExecutable``, as
  executables can now only be created from binaries

  * ``core_executable_options_t``
  * ``core_source_type_e``
  * ``core_source_capabilities_e``
  * ``core_work_item_order_e``
  * ``core_executable_flags_e``

* Compilation specific enum values of ``core_error_t``

  * ``core_error_missing_llvm``
  * ``core_error_missing_clang``
  * ``core_error_compile_failed``
  * ``core_error_linked_failed``

* Snapshot functions

  * ``coreListSnapshotStages``
  * ``coreSetSnapshotStage``

Removal of the runtime compiler
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In Core, a ``core_executable_t`` represented two different things: either a
loaded LLVM module that can be further optimized into machine code, sometimes
called the "runtime" path, or it could store a pre-compiled binary, sometimes
called the "offline" path. You would then create a ``core_kernel_t``, followed
by a ``core_scheduled_kernel_t``, followed by a ``core_specialized_kernel_t``.

In ComputeMux, we have moved the "runtime" path to the new `Compiler`_ API and
have left the "offline" path. This now means that a ``mux_executable_t`` is
simply a container for binary machine code that can run on your device, and all
the kernel types have been combined into ``mux_kernel_t``, which now can be
thought of as a kind of "function pointer" into the ``mux_executable_t``
container.

To migrate your code across, look at your implementation of
``coreCreateExecutable``, and remove all code that depend on ``source_type``
being equal to one of the ``core_source_type_llvm`` enum values, and clean up
any references to that path in your implementation of ``core_executable_t``
and ``core_kernel_t``. You should find at this point that
``core_scheduled_kernel_t`` and ``core_specialized_kernel_t`` no longer do any
work, and thus can be folded into ``mux_kernel_t``. The launch parameters passed
to ``coreCreateScheduledKernel`` and ``coreCreateSpecializedKernel`` are now
provided in ``muxCommandNDRange``, more on this can be found below.

Lastly, for built-in kernels, you should migrate to the new
``muxCreateBuiltInKernel`` endpoint, which allows you to create built-in kernels
directly from the device without having to go through an executable beforehand.
This replaces the functionality of ``coreCreateExecutable`` with the
``core_source_type_builtin_kernel`` source type.

You should now be left with an implementation that can handle binary kernels and
built-in kernels if your target supports this feature.

Renaming and API tweaks
~~~~~~~~~~~~~~~~~~~~~~~

For the most part, the ComputeMux Runtime API is virtually identical to the
Core API apart from some renaming and dropped parameters. However, there
have been some significant changes to ``corePushNDRange``, now called
``muxCommandNDRange``. This section describes those changes in more detail.

All entry points and types have been renamed in the following way:

* The prefix of every entry point has changed from ``core`` to ``mux``. For
  example, ``coreCreateExecutable`` is now called ``muxCreateExecutable``.
* ``corePush`` functions have been renamed to ``muxCommand``. For example,
  ``corePushNDRange`` is now called ``muxCommandNDRange``.
* The prefix of every type and enum has changed from ``core`` to ``mux``. For
  example, ``core_executable_t`` is now called ``mux_executable_t``.
* ``core_command_group_t`` has been renamed to ``mux_command_buffer_t``, and
  associated functions have been renamed from ``core*CommandGroup`` to
  ``mux*CommandBuffer``. For example, ``coreCreateCommandGroup`` has been
  renamed to ``muxCreateCommandBuffer``.
* ``core_error_t`` has been renamed to ``mux_result_t``, and
  ``core_error_success`` has been renamed to ``mux_success``.

A number of structs have had fields changed:

* ``source_capabilities``, ``compilation_options``, ``vectorizable``,
  ``dma_optimizable`` and ``scalable_vector_support`` have been removed from
  ``mux_device_info_s``. All of these except ``source_capabilities`` have been
  moved to ``compiler::Info``, more on this later.
* ``build_options`` has been removed from ``mux_executable_s``.
* ``sub_group_size`` has been moved from ``core_scheduled_kernel_s`` to
  ``mux_kernel_s``, as ``mux_kernel_s`` always represents a binary kernel.

There have also been a few changes to the function signatures of some functions:

* The following functions no longer require a finalizer:

  * ``muxCreateExecutable``
  * ``muxCreateKernel``
  * ``muxDestroyKernel``

* ``muxCreateExecutable`` no longer takes a ``source_type`` or ``build_options``
  argument, as executables are always binaries in ComputeMux. The work to
  migrate to this function should be mostly done already.

* ``muxDestroy*`` functions and ``muxFreeMemory`` no longer return a value. If
  invalid parameters are provided, then the functions are now expected to do
  nothing. This more closely resembles the behaviour of C++ destructors, and
  ``free()``.

* Instead of receiving a specialized kernel, ``muxCommandNDRange`` now takes a
  ``mux_kernel_t`` object directly. The launch parameters previously passed to
  both ``coreCreateScheduledKernel`` and ``coreCreateSpecializedKernel``, such
  as local size, global size and descriptors, is now passed directly to
  ``muxCommandNDRange`` in the ``mux_ndrange_options_t`` struct.

Compiler
--------

By this point you should have a working ComputeMux Runtime target which can load
binary kernels previously compiled by your Core target. The next step is to
integrate the compiler code which was removed during the steps above into the
new ComputeMux Compiler interface.

Start by creating a new subdirectory with an empty ``CMakeLists.txt`` that will
contain your new compiler target. If your Core target used to be in
``modules/core/source``, your new compiler target should be a subdirectory
inside ``modules/compiler/targets``, but like Core targets, your ComputeMux
Compiler target can also exist out-of-tree.

Implement ``compiler::Info``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The first aspect of exposing a compiler is to create a subclass of
``compiler::Info`` (contained in ``compiler/info.h``). This object is a struct
that describes the supported features of a particular compiler implementation,
and what ComputeMux device it targets. ``compiler::Info`` is equivalent to the
compiler specific fields of the old ``core_device_info_s``, such as
``compilation_options``, ``vectorizable``, ``dma_optimizable`` and
``scalable_vector_support``.

``compiler::Info`` introduces a new field called
``supports_deferred_compilation``. Set this to ``false`` for now, as this is an
optional feature.

In addition, the ``compiler::Info`` contains a field called ``device_info``.
This is a ``mux_device_info_t`` that usually points to the device info returned
by ``muxGetDeviceInfos`` that this particular compiler will be targeting. This
is required for two reasons:

1) At runtime, ComputeAorta needs to know which compiler can be used to compile
   for a particular ``mux_device_t``. This is done by comparing the ID's of the
   ``mux_device_info_t`` contained within the ``compiler::Info`` and the current
   ComputeMux device in ``compiler::getCompilerForDevice``.

2) A ComputeMux compiler usually needs to know details about the specific device
   it is targeting, such as floating point capabilities, or endianness.

If your target contains cross compilers i.e. a compiler with no associated
runtime device, the ``device_info`` field can point to an instance of
``mux_device_info_s`` that describes the device it is cross compiling for. In
this case, this doesn't need to be a device info that's returned by
``muxGetDeviceInfos``. The caveat is that this compiler wont be accessible from
the OpenCL or Vulkan driver at runtime, but instead only from an offline
compiler such as ``clc``. An example of cross compilers can be found in the
``host`` implementation in ``modules/compiler/targets/host``.

``compiler::Info`` requires a single virtual method to be implemented:
``createTarget()``. This should return a new instance of ``compiler::Target``
described by this ``compiler::Info``, more on this below in
`Implement compiler::Target`_.

Register available compilers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To expose your compiler to ComputeAorta, ComputeMux requires you to expose a
global function (or static member function) with a single argument of type
``compiler::AddCompilerFn``. Your function will be called by ComputeAorta to
register all included compilers with ComputeAorta. Your function should then
call the callback and pass a pointer to a singleton instance of your
``compiler::Info``. For example:

.. code:: cpp

    void registerCompilers(compiler::AddCompilerFn add_compiler) {
      static MyCompilerInfo info;
      add_compiler(&info);
    }

Generally, you would need to register a compiler for each device returned by
``muxGetDeviceInfos``, and you would also register any cross compilers at this
point.

Lastly, to tell ComputeAorta about your registration function, call the function
:cmake:command:`add_mux_compiler_target` in CMake, and pass the fully qualified
name to your registration function in the ``COMPILER_INFO`` argument, and the
path to the header in the ``HEADER_DIR`` argument. For example:

.. code:: cmake

   add_mux_compiler_target(MyCompiler
     COMPILER_INFO ::registerCompilers
     HEADER_DIR my_compiler/info.h)

Implement ``compiler::Target``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start by creating a new class which extends ``compiler::BaseTarget``.
``compiler::Target`` is an object that represents a compiler instance. This
object is equivalent to ``core_finalizer_s``. The implementation of
``coreCreateFinalizer`` from your Core target should be moved to
``compiler::Target::initWithBuiltins``.

Builtins are no longer passed as a serialized LLVM module to your compiler, as
it was in ``coreCreateFinalizer``. Instead, ownership of an LLVM module is
passed to ``initWithBuiltins()`` for you to use directly.

``compiler::Target`` also includes ``listSnapshotStages``, which can be left as
a stub implementation. We will implement this function later on once we
`implement snapshots`_.


Implement ``compiler::Module``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start by creating a new class which extends ``compiler::BaseModule``.
``compiler::Module`` drives the entire compilation process for a single program.
This object is roughly equivalent to ``core_executable_s`` when created with
LLVM IR that which we removed from the runtime port in an earlier step. All of
the frontend functionality of ``compiler::Module`` (such as OpenCL C, SPIR and
SPIR-V) is already implemented in ``compiler::BaseModule``, so your compiler
does not need to implement these.

The mandatory methods in ``compiler::BaseModule`` are ``getLateTargetPasses``
and ``createBinary``:

* ``compiler::BaseModule::getLateTargetPasses`` is equivalent to the "runtime
  compilation" path in ``coreCreateExecutable``. It should return a
  ``llvm::ModulePassManager`` containing target-specific IR passes to be run on
  the module to prepare it for ``createBinary``. The returned pass manager is
  added to the end of the pipeline built and run in
  ``compiler::BaseModule::finalize``.

* ``compiler::Module::createBinary`` is equivalent to
  ``coreCreateBinaryFromExecutable``.

Note that ``coreCreateBinaryFromSource`` has no equivalent in the Module API, so
can be removed.

Implement snapshots
~~~~~~~~~~~~~~~~~~~

``compiler::BaseModule`` already implements much of the snapshot infrastructure.
For example, it implements the method ``BaseModule::setSnapshotCallback``. This
method is equivalent to ``coreSetSnapshotStage``.
``BaseModule::setSnapshotCallback`` assigns to the protected member
``BaseModule::snapshot_details`` with the details passed to
``BaseModule::setSnapshotCallback``, which can be accessed by your ``Module``
implementation.

In your compilation passes, you can check ``snapshot_details.has_value()`` to
check if a snapshot callback has been set, then ``snapshot_details->stage`` to
check whether the snapshot stage string matches one of your snapshot stages.
For example:

.. code:: cpp

  if (snapshot_details.has_value() &&
      std::string{snapshot_details->stage} == "my_snapshot_stage") {
    // A hypothetical function that serializes the LLVM BC and passes it to the callback.
    TakeASnapshot(llvm_module, *snapshot_details);
  }

``BaseModule`` also provides the function ``BaseModule::takeSnapshot`` and the
``compiler::utils`` module contains a ``compiler::utils::SimpleCallbackPass``,
which can be used to easily invoke a snapshot at a specific point in the
compilation pipeline, for example:

.. code:: cpp

  if (snapshot_details.has_value() &&
      std::string{snapshot_details->stage} == "my_snapshot_stage") {
    pm.addPass(compiler::utils::SimpleCallbackPass([this](llvm::Module &m) {
      takeSnapshot(&m);
    });
  }


Neither of the above are required to be used.

``compiler::Target`` includes ``listSnapshotStages`` which is equivalent
to ``coreListSnapshotStages``. The only difference is that you should also list
the snapshot passes included in ``compiler::BaseModule``. ``BaseModule`` passes
are accessible from the public static member
``compiler::BaseModule::snapshot_stages``. See the ``host`` implementation in
``modules/compiler/targets/host/source/target.cpp`` for an example.

Implement deferred compilation (optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If your Core target performs all compilation inside ``coreCreateExecutable``,
then you are now done. However, if your Core target performs defers any
compilation work to ``coreCreateScheduledKernel`` and/or
``coreCreateSpecializedKernel``, taking information like local size into
account, then the ``compiler::Kernel`` interface needs to also be implemented.

First step is to implement ``compiler::Module::createKernel``. This is
equivalent to the "runtime compilation" path of ``coreCreateKernel``, which
results in a ``compiler::Kernel`` object that is ready for further processing.

Next, you need to create an implementation of ``compiler::Kernel``. This object
can be thought of as a combination of ``core_kernel_t``,
``core_scheduled_kernel_t`` and ``core_specialized_kernel_t``.

The methods are as follows:

* ``compiler::Kernel::createSpecializedKernel`` - this function provides the
  compiler with the properties that this kernel will be executed with, such as
  local size, global size, and descriptors, and returns a binary containing
  the kernel function to be passed to ``muxCreateExecutable``.

  * Note that the only requirement here is that the binary buffer returned by
    ``createSpecializedKernel`` should be loadable by ``muxCreateExecutable``.
    This means that, if required, pointers can be safely passed along to
    ``muxCreateExecutable`` by encoding them inside the binary buffer.

* ``compiler::Kernel::precacheLocalSize`` - ComputeAorta will call this function
  to hint to the compiler that this local size may be used in the future.

* ``compiler::Kernel::getDynamicSubgroupSize`` - this is equivalent to
  ``core_scheduled_kernel_s::dynamic_subgroup_size``, but computed at runtime
  given a desired local size.

To match the scheduled and specialized kernel flow in Core, one way of
implementing deferred compilation would be to create a private helper function
called ``getOrCreateOptimizedKernel`` (equivalent to
``coreCreateScheduledKernel``) that optimizes a kernel for a given local size,
and caches it in a lookup table (such as ``std::map``) keyed by local size. This
function can then be used to implement ``createSpecializedKernel``,
``precacheLocalSize`` and ``getDynamicSubgroupSize``. See
``modules/compiler/targets/host/source/kernel.cpp`` for an example of this
pattern.

Lastly, once this is complete, make sure to set
``supports_deferred_compilation`` to ``true`` in your ``compiler::Info``, to
signal to ComputeAorta that your compiler supports deferred compilation and
the ``compiler::Kernel`` flow should be used.
