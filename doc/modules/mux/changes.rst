ComputeMux Change Log
=====================

The change log contains, **with most recent items at the beginning**,
changes made to the ComputeMux API. Versioning follows the `semantic
versioning <http://semver.org/>`__ scheme; major version increments
signify incompatible API changes, minor version bumps denote the
addition of new functionality which is backward compatible, and patch
version increases mean backward compatible bug fixes have been applied.

   Versions prior to 1.0.0 may contain breaking changes in minor
   versions as the API is still under development.

0.79.0
------

* Added sub-group shuffle builtins.

0.78.0
------

* Added sub-group, work-group, and vector-group operation builtins.

0.77.0
------

* The DMA builtins now permit any event type chosen by the target, as long as
  they're consistent across the module.

0.76.0
------

* Added ``num_sub_group_sizes`` and ``sub_group_sizes`` to ``mux_device_info_s``.


0.75.0
------

* Snapshots have been removed.

0.74.0
------

* Several ``compiler::BaseModule`` methods and fields have been reworked to
  make compilation of OpenCL less stateful.
  * The class-member lists of macro defs and OpenCL options have been removed,
    and are set up and torn down on the fly when compiling OpenCL C.
  * Several methods such as ``compiler::BaseModule::populatePPOpts`` are now
    ``const``.
  * ``compiler::BaseModule::executeOpenCLAction`` has been removed.

0.73.0
------

* Added the ``muxQueryMaxNumSubGroups`` entry point.
* Removed ``mux_kernel_s::sub_group_size``, which is now an implementation
  detail.

0.72.0
------

* Added the ``__mux_mem_barrier``, ``__mux_work_group_barrier``, and
  ``__mux_sub_group_barrier`` builtins. They replace the older
  ``__mux_global_barrier``, ``__mux_shared_local_barrier``, and
  ``__mux_full_barrier`` builtins, which have been removed.

0.71.0
------

* Added the ``mux_device_s::supports_generic_address_space`` field.

0.70.0
------

* ``compiler::BaseModule::runBackendPasses`` and
  ``compiler::BaseModule::addLatePasses`` have been removed. Their
  functionality are covered by both the new
  ``compiler::BaseModule::getLateTargetPasses`` method and the pre-existing
  ``compiler::Module::createBinary``.

0.69.0
------

* Remove ``muxWait`` and move its functionality in ``muxTryWait`` when ``timeout`` is ``UINT64_MAX``.

0.68.1
------

* Require implementations of ``muxCloneCommandBuffer`` to deep copy
  ndrange kernel commands.

0.68.0
------

* Add ``timeout`` parameter to ``muxTryWait``.

0.67.0
------

* Replace ``mux_error_command_buffer_not_ready`` with ``mux_fence_not_ready``.
* Replace ``mux_error_command_buffer_failure`` with ``mux_error_fence_failure``.
* Remove ``mux_error_command_buffer_wait_semaphore_failure``.

0.66.0
------

* Rename all builtin functions from ``__core`` to ``__mux``.

0.65.0
------

* ``compiler::BaseTarget`` now owns the builtins module optionally created in
  ``compiler::BaseTarget::init`` and initialized with the target as part of
  ``compiler::BaseTarget::initWithBuiltins``.

0.64.0
------

* Update ``muxDispatch`` to accept an optional ``mux_fence_t`` parameter.
* Update ``muxTryWait`` to wait on a ``mux_fence_t`` rather than a
  ``mux_command_buffer_t``.
* Update ``muxWait`` to wait on a ``mux_fence_t`` rather than a
  ``mux_command_buffer_t``.

0.63.0
------

* Add ``muxCreateFence``, ``muxDestroyFence`` and ``muxResetFence`` entry
  points.

0.62.0
------

* Add ``mux_sync_point_s`` type, representing intra command-buffer
  synchronization points for ordering commands inside a command-buffer.
  ``MuxCommand*`` entry-points have been updated to return a sync-point, as well
  as taking a list of sync-points to wait on.

0.61.0
------

* A new method ``getBuiltinCapabilities`` has been added to ``compiler::Info``.
  Calling this function will return a bitfield of the builtin capabilities of
  the device, based on the mux device info.

0.60.0
------

* Add the ``muxQueryWFVInfoForLocalSize`` entry point.

0.59.0
------

* Add ``mux_fence_s`` type. There are currently no Mux entry points to create,
  wait on, query, reset or destory ``mux_fence_s`` objects, these will be added
  in a future spec version.

0.58.0
------

* ``BaseModule`` has an additional virtual method ``createPassMachinery()``.
  This will provide a ``PassMachinery`` which can be used throughout the pipeline
  to handle state needed for the new pass manager interface.

0.57.0
------

* Added the ``mux_device_s::supports_work_group_collectives`` field.

0.56.1
------

* Extend valid usage description of ``muxUpdateDescriptors`` to include
  text on changing the size of POD descriptors being undefined behaviour.

0.56.0
------

* Add the following entry points:
  * ``compiler::Kernel::querySubGroupSizeForLocalSize``
  * ``compiler::Kernel::queryLocalSizeForSubGroupCount``
  * ``compiler::kernel::queryMaxSubGroupCount``
  * ``muxQuerySubGroupSizeForLocalSize``
  * ``muxQueryLocalSizeForSubGroupCount``
* Remove the following entry point:
  * ``compiler::Kernel::getSubGroupSize()``
* Add the following fields:
  * ``mux_device_info_s::max_sub_group_count``
  * ``mux_kernel_s::max_sub_group_count``
* Remove the following field:
  * ``mux_device_info_s::max_num_sub_groups``
  * ``mux_kernel_s::sub_group_size``

0.55.0
------

* Add the ``__core_dma_read_3D`` builtin.
* Add the ``__core_dma_write_3D`` builtin.
* Modify ``__core_dma_read_2D`` and ``__core_dma_write_2D`` to handle source
  and destination strides.

0.54.0
------

* ``cargo::optional<mux_device_t> device`` and
  ``mux_allocator_info_t allocator_info`` has been removed from
  ``compiler::Info::createTarget``.
* ``compiler::BaseKernel::createSpecializedKernel`` has been moved to
  ``compiler::Kernel::createSpecializedKernel``.
  ``compiler::Kernel::createMuxSpecializedKernel`` was an implementation detail
  of ``compiler::BaseKernel`` which has now been removed.
* ``compiler::SpecializedKernel`` has been removed.
* ``compiler::BaseTarget`` now loads the builtins module for the given builtin
  capabilities as part of ``compiler::BaseTarget::init``. Compiler targets
  should implement ``compiler::BaseTarget::initWithBuiltins`` instead. Unlike
  ``init``, ``initWithBuiltins`` does not need to delegate to
  ``compiler::BaseTarget`` first, as it's a pure virtual function.
* The notification callback passed to ``compiler::Target::init`` is now passed
  to ``compiler::Info::createTarget`` and is now of type
  ``compiler::NotifyCallbackFn``. This should be passed along to
  ``compiler::BaseTarget``'s constructor.

0.53.2
------

* Change the ``user_function`` argument of ``muxCommandUserCallback`` to use the
  ``mux_command_user_callback_t`` type, rather than the function pointer type
  explicitly.

0.53.1
------

* Remove note mandating that targets do their own validation of ``data`` and
  ``stride`` ``muxGetQueryPoolResults`` parameters.

0.53.0
------

* Add the ``uint32_t mux_query_counter_s::hardware_counters`` field.
* Add the ``uint32_t mux_device_info_s::max_hardware_counters`` field.

0.52.0
------

* Rename member ``max_subgroup_size`` in ``mux_device_info_t`` to
  ``max_work_width``.
* Rename member function ``getDynamicSubgroupSize`` in ``compiler::Kernel`` to
  ``getDynamicWorkWidth``.

0.51.0
------

* Added the ``__core_get_max_sub_group_size()`` builtin.

0.50.0
------

* Version bump to maintain parity with Core which has had the
  ``__core_get_num_sub_groups`` builtin added.

0.49.0
------

* Version bump to maintain parity with Core which has had the
  ``__core_get_sub_group_id`` builtin added.

0.48.0
------

* Add the ``size_t mux_kernel_s::sub_group_size`` field.
* Add the ``cargo::expected<uint32_t, Result>
  compiler::Kernel::getSubGroupSize()`` method.

0.47.0
------

* Add the ``uint32_t mux_device_info_s::max_num_sub_groups`` field.
* Add the ``bool mux_device_info_s::sub_groups_support_ifp`` field.

0.46.0
------

* Add member ``scalable_vector_support`` to ``compiler::Info`` to represent that
  the compiler supports generating scalable vector code.
* Add member ``scalable_vectors`` to ``compiler::Options`` to indicate that the
  executable should be finalized with scalable vectors.

0.45.0
------

* Version bump to maintain parity with Core which has had the
  ``__core_dma_write_2D`` and ``__core_dma_write_2D`` builtins added.

0.44.0
------

* Initial release of the ComputeMux specification. The changelog for the Core
  specification has been duplicated here to preserve history.
* Remove the ``corePushBarrier`` entry point, which was rendered obsolete when
  command groups were guaranteed to execute in order.

0.43.1
------

* Add ``core_source_type_llvm_140`` and ``core_source_capabilities_llvm_140`` for
  supporting LLVM 14

0.43.0
------

* Add the ``coreCloneCommandGroup`` entry point.
* Add the ``bool core_device_info_s::can_clone_command_groups`` field.

0.42.1
------

* Relax thread-safety requirements of implementing ``coreFinalizeCommandGroup()``\ ,
  so that the entry-point is only thread-safe with respect to the same
  command-group handle rather than across all invocations.

0.42.0
------

* Add the ``coreUpdateDescriptors`` entry point.
* Add the ``bool core_device_info_s::descriptors_updatable`` field.

0.41.0
------

* Add the ``coreFinalizeCommandGroup`` entry point.

0.40.3
------

* Add ``core_source_type_llvm_130`` and ``core_source_capabilities_llvm_130`` for
  supporting LLVM version 13.0.0.

0.40.2
------

* Add ``core_source_type_llvm_120`` and ``core_source_capabilities_llvm_120`` for
  supporting LLVM version 12.0.0.

0.40.1
------

* Add the ``size_t __core_get_global_linear_id()`` builtin.
* Add the ``size_t __core_get_local_linear_id()`` builtin.
* Add the ``size_t __core_get_enqueued_local_size(uint)`` builtin.

0.40.0
------

* Remove ``host_pointer`` argument from ``coreAllocateMemory``.
* Remove ``core_allocation_type_use_host`` from ``core_allocation_type_e``.
* Rename ``core_allocation_capabilities_e`` enums
  ``core_allocation_capabilities_alloc_host`` to
  ``core_allocation_capabilities_coherent_host`` and
  ``core_allocation_capabilities_use_host`` to
  ``core_allocation_capabilities_cached_host``.

0.39.3
------

* Require stricter device capability ``core_allocation_capabilities_alloc_host``
  to support entry point ``coreCreateMemoryFromHost``\ , as this implies the device
  architecture has cache coherent memory with host.

0.39.2
------

* Forbid mapping already mapped memory objects with ``coreMapMemory``.
* Specify flushing cache coherent memory as a nop.
* Require ``core_memory_property_host_visible`` as a property of memory objects
  mapped with ``coreMapMemory``.

0.39.1
------

* Add a valid use clarification for ``coreCreateSpecializedKernel``.

0.39.0
------

* Add ``alignment`` argument to ``coreAllocateMemory`` to specify the minimum
  alignment for the allocated memory.
* Add ``handle`` member to ``core_memory_s`` to allow the host runtime a way to
  represent the underlying memory address.
* Add entry point ``coreCreateMemoryFromHost`` to allow APIs to create a
  ``core_memory_t`` device visible object from pre-allocated host memory.

0.38.7
------

* Rename the ``core_vectorization_order_e`` enum to ``core_work_item_order_e``\ ,
  and the enum values to match the ``work_item`` naming.
* Rename the ``vec_order`` field of ``core_executable_options_t`` to
  ``work_item_order``\ , to match the rename of ``-cl-wfv-order`` to ``-cl-wi-order``.
* Upgrade Guidance: ``utils::createHandleBarriersPass()`` must now be passed
  a parameter of type ``enum core_work_item_order_e`` to specify the work item
  dimension priority.

0.38.6
------

* Add ``core_vectorization_order_e`` enum type to represent vectorization
  priority order.
* Add ``vec_order`` field to ``core_executable_options_t`` struct for supporting
  the ``-cl-wfv-order`` extension.

0.38.5
------

* Add ``core_source_type_llvm_110`` and ``core_source_capabilities_llvm_110`` for
  supporting LLVM version 11.0.0.

0.38.4
------

* Add documentation for maximum built-in kernel name length.

0.38.3
------

* Add ``core_source_type_llvm_100`` and ``core_source_capabilities_llvm_100`` for
  supporting LLVM version 10.0.0.

0.38.2
------

* Add ``__core_usefast()`` and ``__core_isembeddedprofile()`` functions as required
  builtins that core targets must replace.
* Added ``core_floating_point_capabilities_full`` flag to
  ``core_floating_point_capabilities_e`` for IEEE-754 compliant representations.

0.38.1
------

* Add flags to ``core_executable_flags_e`` to represent the various OpenCL math
  optimization build options, namely:

  * ``core_executable_flags_mad_enable``
  * ``core_executable_flags_no_signed_zeroes``
  * ``core_executable_flags_unsafe_math_optimizations``
  * ``core_executable_flags_finite_math_only``

0.38.0
------

* Add ``compilation_options`` C string to ``core_device_info_s`` to hold custom
  build options provided by the device.
* Add ``core_executable_options_t`` struct which encapsulates the
  ``core_executable_flags_e`` bitfield and a C string for the name and value of
  any device specific build options passed by the user.
* Redefine ``core_executable_s`` struct to have a ``core_executable_options_t``
  member rather than the ``core_executable_flags_e`` bitfield.
* Redefine ``coreCreateBinaryFromSource()`` and ``coreCreateExecutable()`` to take
  a ``core_executable_options_t`` argument rather than a ``core_executable_flags_e``
  bitfield.

0.37.1
------

* Add ``core_executable_flags_prevec_loop`` and
  ``core_executable_flags_prevec_slp`` enum values to
  ``core_executable_flags_e`` for activation of "early vectorization" passes:

  * Loop Vectorization
  * SLP Vectorization
  * Load/Store Vectorization

0.37.0
------

* Core now accepts 3D descriptions of memory in the ``corePush*Region`` entry
  points, these layouts are passed down to the implementation.

  * Reduce the overhead significantly.
  * Redefine ``core_buffer_region_info_s`` to describe a buffer in 1D, 2D or 3D.
    This design is based on OpenCL's ``clEnqueue*BufferRect`` entry points.

0.36.0
------

* Add support for query counters, extending the mechanism for reporting
  performance statistics to the application by providing a configurable method
  for enabling a set of hardware counters alongside metadata which can be used
  by a profiling visualisation tool to describe the queried data.

  * Extend ``core_query_type_e`` to include ``core_query_type_counter``.
  * Add ``coreGetSupportedQueryCounters()`` to enable applications to discover the
    full list of supported query counters.
  * Add ``core_query_counter_t`` used to describe how to enable and interpret a
    query counter.
  * Add ``core_query_counter_description_t`` used to provide human readable
    metadata about a query counter.
  * Extend ``coreCreateQueryPool`` to accept an array of
    ``core_query_counter_config_t``\ s to select which query counters to enable
    *and* pass through additional target specific counter configuration if
    necessary.
  * Extend ``corePushBeginQuery``\ /\ ``corePushEndQuery`` to accept a ``query_count`` in
    addition to a ``query_index``\ , this allows multiple queries to be enabled at
    once.
  * Add ``core_query_counter_result_t`` used to return the result of a single
    query counter to the application using ``coreGetQueryPoolResults()``.

0.35.0
------

* Add support for queries, a mechanism for targets to report performance
  statistics to the application.

  * The ``core_query_pool_t`` object is used to store the query results,
    ``coreCreateQueryPool()`` and ``coreDestroyQueryPool()`` define the objects
    lifecycle, ``coreGetQueryPoolResults()`` is used to provide the results to the
    application.
  * The ``core_query_type_e`` enumeration defines a set of possible queries,
    currently only ``core_query_type_duration`` is supported and is intended to
    report the start and end timestamps of a command, results are reported using
    the ``core_query_duration_result_t`` object.
  * The ``corePushBeginQuery()`` and ``corePushEndQuery()`` entry points define the
    range of commands for which a ``core_query_pool_t`` is to be used in a
    ``core_command_group_t``\ , ``corePushResetQueryPool()`` is used to zero all query
    results in the spcified range within the ``core_query_pool_t``.

0.34.3
------

* Remove unnecessary member ``vectorize`` from ``core_kernel_t``.

0.34.2
------

* Fix ``core.xml`` comment to state that ``CL_DEVICE_NAME`` is matched with
  ``core_device_info_s::device_name``.

0.34.1
------

* Added ``core_source_capabilities_e::core_source_capabilities_llvm_any`` bit
  mask to match any of the LLVM source capability bits.

0.34.0
------

* Add support for custom buffer descriptors, this allows passing through
  arbitrary data from the user to the Core target in addition to the address
  space provided by the compiler frontend. This includes:

  * The ``custom_buffer_capabilities`` data member of ``core_device_info_s``
    describing which custom buffer capabilities the Core target supports.
  * The ``core_custom_capabilities_e`` enumeration of custom buffer capabilities.
  * The ``core_descriptor_info_custom_buffer_s`` structure to describe the custom
    buffer to the Core target.
  * The ``core_descriptor_info_type_custom_buffer`` enumeration value to specify
    that a descriptor is a custom buffer.

0.33.1
------

* Clarify that whitespace characters other than `` `` are not supported in
  built-in kernel declarations.

0.33.0
------

* Unify snapshot descriptions to favor snapshot "stages" over snapshot "points".
  Rename:

  * ``coreListSnapshotPoints`` to ``coreListSnapshotStages``
  * ``coreSetSnapshotPoint`` to ``coreSetSnapshotStage``

* Specify that passing an invalid snapshot stage name to ``coreSetSnapshotStage``
  **must** return ``core_error_malformed_parameter``.
* Remove ``core_snapshot_type_none`` to make it harder to set an invalid format.
* Rename ``core_snapshot_type_e`` to ``core_snapshot_format_e`` to unify how the
  format information is called and used.
* Introduce ``core_snapshot_format_default`` to unify how the format information
  is used.
* Re-order the parameters of ``coreSetSnapshotStage``\ , i.e., move the
  ``snapshot_format`` parameter before the ``snapshot_callback`` parameter.

0.32.3
------

* Added built-in kernel usage section to the Core ``spec.md`` document.

0.32.2
------

* Clarify syntax for built-in kernel declarations.
* Clarify that ``build_flags`` have no effect on ``coreCreateExecutable`` when the
  source type is ``core_source_type_builtin_kernel``.

0.32.1
------

* Clarify that Core implementations of command groups **must not** access
  signal semaphores of completed command groups they depend on.

0.32.0
------

* Add ``core_callback_info_t`` to support implementations providing detailed
  messages to users about API usage.
* Change ``<client>CreateFinalizer`` to take a ``core_callback_info_t`` parameter to
  support provision of detailed messages about compilation.
* Change ``<client>CreateCommandGroup`` to take a ``core_callback_info_t`` parameter
  to support provision of detailed messages about command execution.

0.31.4
------

* Clarify the error return codes of ``coreCreateExecutable`` and
  ``coreCreateBinaryFromSource`` for unknown or invalid ``source_type`` arguments.

0.31.3
------

* Clarify the valid usage of permitted actions in the ``user_function`` callback
  of ``coreDispatch``.
* Clarify when a command group passed to ``coreDispatch`` is considered complete.

0.31.2
------

* Add allocator validity check to ``id.h`` and rename it to ``utils.h``.

0.31.1
------

* Weaken requirement that host-side allocations **must** use the user
  provided allocator to that they **should** use it. This enables use of
  third-party libraries, like LLVM or the C standard library, which do not
  support user provided allocators and should not affect existing target
  implementations.

0.31.0
------

* Supersede ``generate_core_header`` with ``add_core_target``\ , this also simplifies
  the mechanism by which targets register themselves and how they specify their
  capabilities in addition to creating a CMake target to generate the core
  target header.
* Add ``add_core_cross_compilers`` which simplifies the mechanism for registering
  a targets cross-compilers with the ``cross`` target.

0.30.0
------

* Add requirement that commands in a command group must be executed in the order
  they were pushed onto the command group, making command groups in-order.
* Add addition valid usage requirements for the usage ``core_semaphore_t``
  defining when it can be reset and destroyed relating to the lifetime of a
  ``coreDispatch()``.

0.29.2
------

* Changed ``builtin_kernel_names`` to ``builtin_kernel_declarations`` to better
  represent what information is contained.

0.29.1
------

* Numerous clarifications and inconsistencies corrected in the specification and
  Doxygen comments of ``core.h``.

0.29.0
------

* Add ``core_device_type_compiler`` to ``core_device_type_e`` to represent a target
  which only implements the compilation entry points for use in compiling
  offline and cross-compiled kernels.
* Change ``core_device_type_e`` enumerations to make them usable in a bitfield and
  add ``core_device_type_all`` for selecting all device types.
* Change ``coreGetDeviceInfos`` to take a bitfield of ``core_device_type_e`` in
  order to selectively initialize only desired devices.

0.28.4
------

* Changed type of ``device`` member variable in ``core_finalizer_s`` from
  ``core_device_t`` to ``core_device_info_t``.

0.28.3
------

* Add ``core_source_type_llvm_80`` and ``core_source_capabilities_llvm_80`` for
  supporting LLVM version 8.0.0.

0.28.2
------

* Add back in the removed ``id`` member from the ``core_device_s`` struct to fix
  compilation failures in ``coreSelect.h`` when multiple targets are registered.

0.28.1
------

* Add support for builtin kernels to core.
* Added ``core_source_type_unknown``\ , ``core_source_type_builtin_kernel`` and
  ``core_source_capabilities_builtin_kernel`` to ``core_source_type_e`` and
  ``core_source_capabilities_e``.
* Added ``core_source_type_builtin_kernel`` as one of the supported types to
  ``coreCreateExecutable`` for creation of a ``core_executable`` with builtin kernels.
* Reordered values in ``core_source_type_e`` and ``core_source_capabilities_e``.

0.28.0
------

* Changed ``coreCreateFinalizer`` and ``coreDestroyFinalizer`` entrypoints to take
  ``core_device_info_t``\ s instead of ``core_device_t``\ s.
* Added a new type ``core_binary_t``.
* Removed ``coreGetBinary`` and replaced it with a new
  ``coreCreateBinaryFromExecutable`` entrypoint.
* Added ``coreCreateBinaryFromSource`` entrypoint for offline/cross-compilation
  support.
* Added a matching ``coreDestroyBinary`` to destroy binaries created by the above
  two functions.

0.27.0
------

* Separate device enumeration from initialization by adding a new structure:
  ``core_device_info_t``\ , and a new function: ``coreGetDeviceInfos``.
* ``coreCreateDevices`` hook API has changed - a new hook for ``coreGetDeviceInfos``
  was added, which has an almost identical interface to the existing
  ``coreCreateDevices`` hook.

0.26.1
------

* Add ``core_executable_flags_dma_never`` and
  ``core_executable_flags_vectorize_never`` enum values to
  ``core_executable_flags_e``\ , so that the core implementations are informed of
  whether the user chose explicitly to enable/disable these optimizations, or
  if the default behavior is to be used when neither the ``never`` nor ``always``
  flags are present.

0.26.0
------

* Add member ``endianness`` to ``core_device_t`` to represent whether the device
  is big- or little-endian.

0.25.0
------

* Change to CMake to build only the required builtins based on target
  capabilities. Capabilities must be reported in a ``<target_name>_CAPABILITIES``
  variable.

0.24.2
------

* Change the CMake mechanism to generate ``<client>`` API headers, it is now
  possible to override the ``clang-format`` executable used during header
  generation.

0.24.1
------

* Change references to ``command_buffer`` in Doxygen documentation and parameter
  variable names to ``command_group``.

0.24.0
------

* Add member ``dma_optimizable`` to ``core_device_t`` to represent that DMA
  optimizations can be performed for this device.
* Add ``core_executable_flags_dma_always`` to ``core_executable_flags_e`` to
  represent that DMA optimizations must be performed.

0.23.0
------

* Add a new command ``<client>ResetSemaphore()`` to reset a semaphore such that it
  has no previous signalled state.

0.22.5
------

* Add member ``image2d_array_writes`` to ``core_device_t``.

0.22.4
------

* Add member ``integer_capabilities`` to ``core_device_t``.
* Add enum ``core_integer_capabilities_e``.

0.22.3
------

* Add member ``vectorizable`` to ``core_device_t`` to represent that vectorization
  can be performed for this device.
* Add member ``vectorize`` to ``core_kernel_t``.
* Add ``core_executable_flags_vectorize_always`` to ``core_executable_flags_e`` to
  represent that vectorization must be performed.

0.22.2
------

* Add ``core_executable_flags_denorms_may_be_zero`` to ``core_executable_flags_e``
  to represent that denormal floats may be flushed to zero.

0.22.1
------

* Added member ``local_memory_size`` to ``core_kernel_t``.

0.22.0
------

* Add a new command ``<client>PushBarrier()`` to enforce the execution order of
  commands within a command group.

0.21.0
------

* Add a ``core_finalizer_t`` argument to ``<client>DestroyExecutable()``\ ,
  ``<client>DestroyKernel()`` and ``<client>DestroyScheduledKernel()``. Note that
  ``<client>DestroySpecializedKernel()`` does **not** take a ``core_finalizer_t``.

0.20.5
------

* Add ``core_source_type_llvm_70`` and ``core_source_capabilities_llvm_70`` for
  supporting LLVM version 7.0.0.

0.20.4
------

* Remove dead symbol references in Doxygen documentation.

0.20.3
------

* Add ``allocation_size`` to ``core_device_s`` to represent the maximum size of a
  single memory allocation.

0.20.2
------

* Add ``__core_get_work_dim()``\ , ``__core_get_group_id()``\ ,
  ``__core_get_global_id()``\ , ``__core_get_local_id()``\ , ``__core_get_num_groups()``\ ,
  ``__core_get_global_size()``\ , ``__core_get_local_size()``\ ,
  ``__core_get_global_offset()``\ , ``__core_full_barrier()``\ ,
  ``__core_shared_local_barrier()``\ , and ``__core_global_barrier()``\ , required
  builtins that core targets must replace.

0.20.1
------

* Add ``core_source_type_llvm_60`` and ``core_source_capabilities_llvm_60`` for
  supporting the latest version of LLVM.

0.20.0
------

* Add ``<client>PushReadBufferRegions()`` to allow for multiple regions within a
  source buffer to be copied to a destination host pointer.
* Add ``<client>WriteCopyBufferRegions()`` to allow for multiple regions within a
  host pointer to be copied to a destination buffer.
* Add ``<client>PushCopyBufferRegions()`` to allow for multiple regions within a
  source buffer to be copied to a destination buffer.
* Add ``core_buffer_regions_info_s`` as a helper struct to specify to the new
  entry points above what source offset, destination offset, and size to use for
  each region.

0.19.2
------

* Add ``max_subgroup_size`` to ``core_device_s`` to represent the maximum subgroup
  size for kernels on a device, and ``dynamic_subgroup_size`` to
  ``core_scheduled_kernel_s`` to represent the actual subgroup size for that
  scheduled kernel.

0.19.1
------

* Add ``core_source_type_llvm_50`` to ``core_source_flags_e`` to allow input
  binaries to be from LLVM 5.0.
* Add ``core_source_capabilities_llvm_50`` to ``core_source_capabilities_e`` to
  allow input binaries to be from LLVM 5.0.

0.19.0
------

* Add ``core_device_t`` argument to create entry points which were not already
  passed a device making the API consistent across all create and destroy
  functions.

0.18.1
------

* Add ``__core_dma_read_1d()``\ , ``__core_dma_read_2d()``\ , and ``__core_dma_wait()``
  functions as builtins that core targets must replace if they use the automatic
  DMA.

0.18.0
------

* Add ``core_allocator_info`` argument to all entry points which perform host
  allocations to support Vulkan style user allocator override.
* Change order of entry points so that ``<client>Create<Object>`` is directly
  before ``<client>Destroy<Object>``.

0.17.3
------

* Add ``compute_units`` to ``core_device_s`` to let implementations pass information
  on how many compute units their device has.

0.17.2
------

* Add ``device_priority`` to ``core_device_s``. This is used to keep track of device
  priorities when returning default devices.

0.17.1
------

* Add ``__core_isftz()`` function as a required builtin that core targets must
  replace.

0.17.0
------

* Add support for multiple memory heaps.
* Add ``supported_heaps`` bitfield to ``core_memory_requirements_s`` allowing the
  client target to state which heaps are supported for a specific buffer or
  image.
* Change ``core_buffer_t`` to have a ``memory_requirements`` data member, replacing
  ``size`` and adding support for specifying ``alignment`` and ``supported_heaps``.
* Add ``heap`` argument to ``<client>AllocateMemory`` to specify the heap to
  allocate memory from.

0.16.0
------

* Added ``native_vector_width`` and ``preferred_vector_width`` to ``core_device_t`` to
  let devices expose what vector width (in bytes) their hardware is, and what
  size of vectors they would prefer implementations give them.

0.15.0
------

* Added ``preferred_local_size_x``\ , ``preferred_local_size_y``\ , and
  ``preferred_local_size_z`` to ``core_kernel_t`` to let implementations pass
  information on what would be a suitable local work group size to use for a
  given kernel.

0.14.0
------

* Removed ``<client>PushTerminate()`` as it put a higher burden on client targets
  than was necessary.

0.13.0
------

* Add ``<client>GetBinary()`` to retrieve the binary representation of a
  ``core_executable_t``.
* Add ``core_source_type_binary`` to ``core_source_flags_e`` to allow the input to
  be a binary for the given core target.
* Add ``core_source_capabilities_binary`` to ``core_source_capabilities_e`` to allow
  a core target to advertise it can support creating executables from binaries.
* Rename ``<client>CreateQueue()`` to ``<client>GetQueue()`` and change the function
  signature to take two extra parameters for the queue type and index.
  ``core_queue_t``\ 's now belong to the device, and are queried from the device,
  rather than an arbitrary number of them being created (which simplifies the
  engineering effort required by our customers).
* Add new enum ``core_queue_type_e`` to denote all possible types of queue we can
  support - at present this only contains ``core_queue_type_compute``\ , but is
  available for extension later.
* Add new field to ``core_device_t`` to query the number of queues of each
  ``core_queue_type_e`` a device supports.
* Remove ``<client>DestroyQueue()``\ , as queues are now implicitly destroyed when
  the device they were retrieved from is destroyed.

0.12.4
------

* Fix bug in ``core::util::allocator::create`` where references were not correctly
  passed through to the constructor of the object being created.

0.12.3
------

* Add ``core_source_type_llvm_40`` to ``core_source_flags_e`` to allow input
  binaries to be from LLVM 4.0.
* Add ``core_source_capabilities_llvm_40`` to ``core_source_capabilities_e`` to
  allow input binaries to be from LLVM 4.0.

0.12.2
------

* Add ``core_executable_flags_no_opt`` to ``core_executable_flags_e``.
* Change semantics of ``core_executable_flags_debug`` to mean built with debug
  info.

0.12.1
------

* Add ``core_executable_flags_soft_math`` to ``core_executable_flags_e`` to force
  finalization to occur using software math builtins.

0.12.0
------

* Add ``max_work_group_size_x``\ , ``max_work_group_size_y`` and
  ``max_work_group_size_z`` to ``core_device_t``.

0.11.1
------

* Add ``CORE_NULL_ID`` preprocessor definition to be used by clients when
  initializing ``core_<object>_s::id``.

0.11.0
------

* Add ID types ``core_id_t``\ , ``core_object_id_t``\ , ``core_target_id_t``.
* Generate ``core_target_id_e`` enum in ``core/coreConfig.h`` from list of
  registered targets.
* Add ``core_id_t id`` member to all objects created by clients.
* Add missing ``core_device_t`` parameter to ``<client>ListSnapshotPoints``.
* Add ``core/util/id.h`` utility header for working with object ID's.

0.10.0
------

* Added ``builtins_type``\ , ``builtins``\ , and ``builtins_length`` parameters to
  ``<client>CreateFinalizer()`` to pass the compute APIs standard library to the
  core client target for linking. Client targets must now link in the builtin
  function definitions themselves to use our provided implementations. By moving
  the responsibility for linking to the client target, clients now have a
  mechanism to intercept any of the builtin functions with target specific
  optimizations, before linking in any remaining builtins that the client does
  not have optimized support for.

0.9.0
-----

* Remove no longer required ``page_size`` from ``core_device_t``.
* Renamed ``core_descriptor_info_shared_scratch_s`` to
  ``core_descriptor_info_shared_local_buffer_s`` to be more consistent with our
  naming.
* Renamed ``core_descriptor_info_type_shared_scratch`` to
  ``core_descriptor_info_type_shared_local_buffer`` to be more consistent with our
  naming.

0.8.1
-----

* Add overload to ``core::allocator::alloc()`` which takes a non-template
  alignment parameter.

0.8.0
-----

* Add ``image3d_writes`` flag to ``core_device_s`` to signify support for writing to
  3D images.

0.7.0
-----

* Add ``<client>FlushMappedMemoryToDevice()`` to synchronize device memory with
  data currently residing in host memory.
* Add ``<client>FlushMappedMemoryFromDevice()`` to synchronize host memory with
  data currently residing in device memory.
* Remove ``flags`` parameter to ``coreMapMemory()``\ , use
  ``<client>FlushMappedMemoryToDevice()`` and
  ``<client>FlushMappedMemoryFromDevice()`` to perform flushing instead.
* Remove ``core_mapping_type_e``\ , ``coreMapMemory()`` and ``coreUnmapMemory()`` are no
  longer required to synchronize memory.

0.6.2
-----

* Remove ``max_instructions_issued_per_cycle`` from ``core_device_s`` as it is no
  longer a required (or useful) piece of functionality to require our customers
  to guestimate.

0.6.1
-----

* Change ``core_source_type_e`` and ``core_source_capabilities_e`` to be the LLVM
  version of the bitcode module being passed in (which more correctly fits our
  usage).
* LLVM bitcode modules being passed in with ``core_source_type_llvm_38`` and
  ``core_source_type_llvm_39`` must have the "unknown-unknown-unknown" target
  triple now.

0.6.0
-----

* Add function ``<client>ListSnapshotPoints`` to retrieve the list of compilation
  stages snapshots can be taken at in partner code.
* Add function ``<client>SetSnapshotPoint`` to set a snapshot point in partner
  code.
* Add enum ``core_snapshot_type_e`` to describe snapshot formats.
* Add typedef ``core_snapshot_callback_t`` to describe the function prototype for
  the callback invoked when a snapshot point is hit.

0.5.0
-----

* Add struct ``core_semaphore_s`` representing a device semaphore object.
* Add function ``<client>CreateSemaphore`` to create device semaphore objects.
* Add function ``<client>DestroySemaphore`` to destroy device semaphore objects.
* Add function ``<client>TryWait`` to try and wait on command groups.
* Change ``<client>Dispatch`` to include two arrays of semaphores, one to wait on
  before beginning execution of the command group, and one to signal when the
  command group has completed executing.
* Change ``<client>Dispatch`` to include a command group complete callback and
  user data.
* Add ``core_error_command_group_failure`` to ``core_error_e`` enum to signal that a
  command group that was waited on failed.
* Add ``core_error_command_group_wait_semaphore_failure`` to ``core_error_e`` enum
  to signal that a command group that was waiting on another command group via a
  semaphore failed because the other command group failed.
* Add ``core_error_command_group_not_ready`` to ``core_error_e`` enum to signal that
  a command group that was waited on was not yet complete.
* Add extra parameter to ``<client>PushFillImage`` to specify the size of the user
  memory being passed in as the color parameter.
* Add function ``<client>PushTerminate`` to signal that a command group should
  terminate, and any semaphore in the chain of waits on it, should not execute.
* Add function ``<client>ResetCommandGroup`` to reset a command group such that it
  has no previous commands enqueued within it.

0.4.0
-----

* Add struct ``core_image_s`` representing a device image object.
* And struct ``core_sampler_s`` representing a device sampler object.
* Update struct ``core_device_s`` to contain the devices image capabilities.
* Change enum ``core_memory_type_e`` into ``core_memory_property_e`` to describe the
  desired memory properties for an allocation, ``core_memory_type_e`` was too
  restrictive and did not allow implementation of
  ``CL_MEM_OBJECT_IMAGE1D_BUFFER``.
* Add struct ``core_memory_requirements_s`` to describe the device memory
  allocation requirements of a ``core_buffer_t`` or a ``core_image_t``.
* Add struct ``core_offset_3d_t`` to describe the offset into an image.
* Add struct ``core_extent_3d_t`` to describe the region of an image.
* Add enum ``core_image_type_e`` to describe the type of an image.
* Add enum ``core_image_format_e`` to describe the format on an image.
* Add enum values ``core_descriptor_info_type_image`` and
  ``core_descriptor_info_type_sampler`` to ``core_descriptor_info_type_e``.
* Add enum ``core_address_mode_e`` to describe sampler addressing modes.
* Add enum ``core_filter_mode_e`` to describe sampler filter modes.
* Change ``<client>AllocateMemory`` to accept a bitfield of
  ``core_memory_property_e``
* Add function ``<client>CreateImage`` to create device image objects.
* Add function ``<client>DestroyImage`` to destroy device image objects.
* Add function ``<client>BindImageMemory`` to bind device memory to an image
  object.
* Add function ``<client>GetSupportedImageFormats`` to query the device for
  supported image formats.
* Add function ``<client>PushReadImage`` to read an image in a command group.
* Add function ``<client>PushWriteImage`` to write an image in a command group.
* Add function ``<client>PushFillImage`` to fill an image in a command group.
* Add function ``<client>PushCopyImage`` to copy and image to another in a command
  group.
* Add function ``<client>PushCopyImageToBuffer`` to copy an image to a buffer in a
  command group.
* Add function ``<client>PushCopyBufferToImage`` to copy a buffer to an image in a
  command group.

0.3.1
-----

* Fixed ``core_memory_type_e`` - it should have been a bitfield.
* Fixed core.h C compilation issue (enum types are called ``enum <type>``).

0.3.0
-----

* Added enum ``core_executable_flags_e`` for build flags.
* Added ``build_flags`` field to executable representing compilation/linking
  options set for the module.
* Added ``build_flags`` parameter to function ``<client>CreateExecutable``.

0.2.0
-----

* Add handle ``core_memory_t`` to take sole ownership of device memory allocations
  in preparation for image support.
* Add struct ``core_memory_s``.
* Add functions ``<client>AllocateMemory`` and ``<client>FreeMemory`` to handle
  device memory allocations.
* Add function ``<client>BindBufferMemory`` to associate a device memory
  allocation with a buffer object. This also adds first class support to the API
  for ``clCreateSubBuffer``.
* Add enum ``core_memory_type_e`` used to specify if an allocation should support
  buffers, images, or both buffers and images. Add typedef to the definition to
  allow passing as a function parameter.
* Combine ``core_buffer_mapping_type_e`` and ``core_buffer_unmapping_type_e`` and
  rename the enum to ``core_mapping_type_e``. Add typedef to definition to allow
  passing as a function parameter.
* Simplify function ``<client>CreateBuffer`` to remove allocation specific
  parameters.
* Add ``core_device_t`` parameter to function ``<client>DestroyBuffer``.
* Remove functions ``<client>MapBuffer`` and ``<client>UnmapBuffer``\ , this
  functionality now applies to ``core_memory_t`` allocations.
* Add functions ``<client>MapMemory`` and ``<client>UnmapMemory`` replacing the
  buffer specific variety.
* Remove member ``device`` from struct ``core_buffer_s``\ , ``device`` is now passed to
  API functions instead.

0.1.3
-----

* Fix documentation for API function ``<client>CreateSpecializedKernel``.

0.1.2
-----

* Removed ``CORE_DEVICE_KHRONOS_CODEPLAY_ID`` and
  ``CORE_DEVICE_KHRONOS_CODEPLAY_NAME`` as they are specific to the Codeplay
  backends.
* Added enum ``core_floating_point_capabilities_e`` for floating point support.
* Added ``half_capabilities`` to device for what half floating point mode is
  supported.
* Added ``float_capabilities`` to device for what floating point mode is
  supported.
* Added ``double_capabilities`` to device for what double floating point mode is
  supported.
* Added enum ``core_shared_local_memory_type_e`` for local memory types.
* Added ``shared_local_memory_type`` to device for the type of shared local memory
  the device supports.
* Added ``shared_local_memory_size`` to device for the size of the shared local
  memory the device has.

0.1.1
-----

* Added enum ``core_cache_capabilities_e`` for read/write caching.
* Added ``cache_capabilities`` field to device for what caching is supported.
* Added ``cache_size`` field to device for the size of the cache supported.
* Added ``cacheline_size`` field to device for the length of a line within the
  cache.

0.1.0
-----

* Replace ``<client>_hook`` with ``<client>CreateDevices``\ , adding support for
  multiple devices per target.

0.0.0
-----

* Add version to XML schema and generated headers.
* Add compile time check for matching versions of all registered targets.
