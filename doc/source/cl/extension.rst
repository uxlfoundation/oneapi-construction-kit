OpenCL Extensions
=================

OpenCL extensions are split into three categories:

KHR
  Extension ratified by Khronos

EXT
  Extension with collaboration from multiple vendors, but not ratified by
  Khronos.

Vendor
  Extension defined by a single vendor, but may be implemented by
  other vendors, e.g oneAPI Construction Kit implements vendor extension
  ``cl_intel_unified_shared_memory``.

oneAPI Construction Kit implements several Codeplay vendor extensions,
specified under the ``extension`` directory. When adding a new vendor
extension the official OpenCL-Docs `cl_extension_template`_ should be used
for reference, but written in RST rather than ASCIIdoc.

When defining new enum valus for an extension, if those enums will be used in
existing entry-points, then they need to be unique to avoid conflicts with enums
defined by other vendors. To enable this vendors reserve a range of values in
16-bit blocks in `cl.xml`_
Codeplay's unique range is between ``0x4260`` & ``0x426F`` inclusively. If our
needs exceed this range then another block can be reserved, although it may not
be contiguous.

.. important::
  The next available value in our range is `0x4263`. Once this value is claimed
  update this text to the new value which is free to use next.

.. _cl_extension_template:
  https://github.com/KhronosGroup/OpenCL-Docs/blob/master/extensions/cl_extension_template.asciidoc

.. _cl.xml:
  https://github.com/KhronosGroup/OpenCL-Docs/blob/master/xml/cl.xml

.. toctree::
  :hidden:
  :maxdepth: 1

  extension/cl_codeplay_kernel_debug
  extension/cl_codeplay_extra_build_options
  extension/cl_codeplay_kernel_exec_info
  extension/cl_codeplay_performance_counters
  extension/cl_codeplay_program_snapshot
  extension/cl_codeplay_soft_math
  extension/cl_codeplay_wfv
  extension/cl_intel_unified_shared_memory

OpenCL C 1.2 - ``khr_opencl_c_1_2``
-----------------------------------

The [OpenCL 1.2 extension specification][cl-12-ext] specifies the following set
of OpenCL C extensions. These extensions have been grouped together into the
`khr_opencl_c_1_2` extension object.

* `cl_khr_global_int32_base_atomics`
* `cl_khr_global_int32_extended_atomics`
* `cl_khr_local_int32_base_atomics`
* `cl_khr_local_int32_extended_atomics`
* `cl_khr_byte_addressable_store`
* `cl_khr_fp64`

The `include/extension/khr_opencl_c_1_2.h` header file and
`source/extension/khr_opencl_c_1_2.cpp` source file define how the extensions
listed above are reported to the OpenCL application by extending the
`clGetDeviceInfo`_ entry point. This extension does not provided any additional
OpenCL API entry points.

Standard Portable Intermediate Representation (SPIR) - ``cl_khr_spir``
----------------------------------------------------------------------

The `SPIR 1.2 specification`_ defines a binary representation of an OpenCL C
program. The ``khr_spir`` extension extends OpenCL to allow creation of a
``cl_program`` from a SPIR binary.

.. _SPIR 1.2 specification:
  https://www.khronos.org/registry/SPIR/specs/spir_spec-1.2.pdf

Installable Client Driver (ICD) - ``cl_khr_icd``
------------------------------------------------

The ICD is provided by Khronos to allow multiple hardware vendor's drivers to
coexist on the same system and avoid an application suffering from linker errors
when attempting to link against those drivers.

The OpenCL entry point definitions do not actually contain the implementation,
where the work is done, instead they call into the ``cl`` namespace. For each
OpenCL entry point, such as `clGetPlatformIDs`_. There is a matching function
in the ``cl`` namespace that is invoked, in this example
``cl::GetPlatformIDs``. This was done to provide a clean boundary between the
ICD and the implementation of the OpenCL entry points.

.. code-block:: cpp

  CL_API_ENTRY cl_int CL_API_CALL clGetPlatformIDs(const cl_uint num_entries,
                                                   cl_platform_id *platforms,
                                                   cl_uint *const num_platforms) {
    return cl::GetPlatformIDs(num_entries, platforms, num_platforms);
  }

Any object created by the driver, such as ``cl_command_queue``, must contain a
pointer to the ICD dispatch table. This is the mechanism used by the ICD to
determine which driver an API object works with; it functions in much the same
way as a C++ virtual function table but because of this similarity the OpenCL
API objects must not themselves contain a virtual function table. The ICD
specifies that any object created by the driver must reserve the first
``sizeof(void*)`` bytes in its structure for the ICD dispatch table.

.. _clGetPlatformIDs:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetPlatformIDs

Kernel Debug - ``cl_codeplay_kernel_debug``
-------------------------------------------

The :doc:`extension/cl_codeplay_kernel_debug` extension grants developers the
ability to specify build options which enable attaching a debugger to a kernel
being executed on a device.

* The ``-g`` flag, providing emission of debug symbols in the compiled kernel.
* The ``-S <path>`` flag, setting the source code location in debug info to the
  specified path. Our runtime then creates this file if it does not already
  exist on disk, allowing the debugger to display kernel source code without
  manual configuration.

Extra build options - ``cl_codeplay_extra_build_options``
---------------------------------------------------------

The :doc:`extension/cl_codeplay_extra_build_options` extension allows the user
to specify additional flags handled by the `clBuildProgram`_ and
`clCompileProgram`_ entry-points.

* The ``-cl-llvm-stats`` flag allowing llvm to print the statistics from all the
  passes that have any.
* The ``-cl-precache-local-sizes=<sizes>`` build option allows for the pre-caching
  of kernel compilation for the specified local work group sizes.

Kernel Exec Info - ``cl_codeplay_kernel_exec_info``
---------------------------------------------------

The :doc:`extension/cl_codeplay_kernel_exec_info` extension adds support for
allowing additional information other than argument values to be passed to a
kernel. This base extension doesn't provide support for any particular parameter
types but is intended to be built upon by future extensions that require this
support. For example Intel USM extension
:doc:`extension/cl_intel_unified_shared_memory` requires support for
`clSetKernelExecInfo`_ which is part of the 2.0 API.  Instead, USM will be able
to use this extension to support 1.2.

.. code-block:: c

   cl_int clSetKernelExecInfoCODEPLAY(cl_kernel kernel,
                                      cl_kernel_exec_info param_name,
                                      size_t param_value_size,
                                      const void* param_value)

.. _clSetKernelExecInfo:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clSetKernelExecInfo

Program Snapshot - ``cl_codeplay_program_snapshot``
---------------------------------------------------

The :doc:`extension/cl_codeplay_program_snapshot` extension provides a snapshot
mechanism allowing user code to capture program objects at different stages of
compilation.

The snapshot mechanism is the primary method of debugging compiler
transformations. This is done by dumping the program object (taking a snapshot)
at a specific stage of compilation in order to inspect the effects of compiler
passes, allowing the user to more effectively debug and tune their code.

OpenCL API
##########

oneAPI Construction Kit has implemented snapshots as an extension to OpenCL. If
the extension is enabled, then `clGetDeviceInfo`_ queries for
``CL_DEVICE_EXTENSIONS`` contain ``"cl_codeplay_program_snapshot"`` as a query
value.

Our two snapshot extension functions can be retrieved by querying
`clGetExtensionFunctionAddressForPlatform`_ for their function name, returning a
functor if the snapshot extension is supported or a NULL pointer otherwise.

The header ``cl_ext_codeplay.h`` containing definitions for Codeplay vendor
extensions declares the required types and functions for use by users.

.. _clGetExtensionFunctionAddressForPlatform:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetExtensionFunctionAddressForPlatform

ComputeMux API
##############

The order of listed snapshots is sorted based on compilation order, where the
first items displayed originate from the internal compiler module in ComputeMux.
With the exception to the ordering of the first entry
``cl_snapshot_compilation_default`` which returns a snapshot at the same stage
as would be returned by ``clGetProgramInfo``. Afterwards the partner
implementation snapshot stages are listed, as they are retrieved via
``compiler::Target::listSnapshotStages``.

Setting the snapshot itself in partner code is done via
``compiler::Module::setSnapshotCallback``, which is invoked in OpenCL right
before the call to ``compiler::Module::finalize``. Passing in the stage name C
string and callback functor. It is an error to pass through a stage name which
doesn't exist in the list returned by ``compiler::Target::listSnapshotStages``.
But it is also valid for this list to be empty when the partner implementation
does not define any snapshot stages at all.

At most a single snapshot stage can be active at any given moment in time based
on the last call to ``compiler::Module::setSnapshotCallback``.

oclc
####

As a developer the easiest way to interact with the snapshot mechanism is via
the `oclc` frontend for compiling OpenCL programs. See the `oclc`
[README](tools.md) for more details.

To see all the available snapshot stages for a particular program a developer
can use the `-list` option:

```sh
oclc <path/to/cl/file> -list
```

Then to place a snapshot the developer must set it on a specific stage with
`-stage`. `oclc` will then print the snapshot to `stdout` if the format is
textual, or to a file if it's binary.

```sh
oclc <path/to/cl/file> -stage cl_snapshot_compilation_spir
```

Sometimes a snapshot stage may not be hit if it requires a kernel be enqueued.
To enqueue a kernel in `oclc` the `-enqueue` option can be used with a specified
kernel name to enqueue it as a task(single work item). The kernel won't actually
be run however, which is expected behaviour due to user events inside `oclc`.

```sh
oclc <path/to/cl/file> -stage cl_snapshot_host_scheduled -enqueue <kernel name>
```

Performance Counter - ``cl_codeplay_performance_counters``
----------------------------------------------------------

The :doc:`extension/cl_codeplay_performance_counters` extension allows the
application to enable and get results from a set of supported, likely
hardware, performance counters.

The flow of execution is as follows:

1.  Query the devices' supported performance counters using `clGetDeviceInfo`_
    with a ``param_name`` of ``CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY``.
2.  Create a ``cl_command_queue`` using `clCreateCommandQueueWithProperties`_
    with a ``properties`` key of ``CL_QUEUE_PERFORMANCE_COUNTERS_CODEPLAY`` and
    a value of type ``cl_performance_counter_config_codeplay*`` containing a
    list of ``cl_performance_counter_desc_codeplay`` structures specifying
    which performance counters should be enabled using ``uuid``\ s attained in
    step 1.
3.  Enqueuing a workload ensuring to provide a ``cl_event``.
4.  Waiting for the workload to complete execution.
5.  Querying the performance counter results using `clGetEventProfilingInfo`_
    with a ``param_name`` of
    ``CL_PROFILING_COMMAND_PERFORMANCE_COUNTERS_CODEPLAY``.
6.  Read the performance counter results using the ``storage`` member of
    ``cl_performance_counter_codeplay`` to select the correct anonymous union
    member of each ``cl_performance_counter_result_codeplay`` structure.

.. _clGetDeviceInfo:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetDeviceInfo
.. _clCreateCommandQueueWithProperties:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clCreateCommandQueueWithProperties
.. _clGetEventProfilingInfo:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetEventProfilingInfo

Soft Math - ``cl_codeplay_soft_math``
-------------------------------------

The :doc:`extension/cl_codeplay_soft_math` extension allows a developer to force
the math builtins used to be sourced from Abacus. An additional build option is
supported for `clCompileProgram`_ and `clBuildProgram`_ to specify this
``"-codeplay-soft-math"``.

When a customer compiler backend consumes an executable, it is free to replace
builtin functions with the optimized equivalents for the target platform. For
instance, even though Abacus provides a conformant implementation of the count
leading zeros ``clz`` builtin function, many mux targets will have a hardware
instruction that allows this function to be implemented more efficiently. With
the ``"-codeplay-soft-math"`` option specified, the mux backend will not use
any hardware optimized builtins, and instead rely on the Abacus functionality.

Being able to specify that the mux target cannot use a more efficient
implementation of Abacus builtin functionality allows testing and performance
metrics to be performed much more easily.

.. _clCompileProgram:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clCompileProgram
.. _clBuildProgram:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clBuildProgram

Whole Function Vectorization - ``cl_codeplay_wfv``
--------------------------------------------------

The :doc:`extension/cl_codeplay_wfv` extension provides a mechanism to
vectorize an OpenCL across the primary work-item dimension, using our whole
function vectorization library VECZ.

An additional build option ``-cl-wfv={always|auto|never}`` is supported for
`clCompileProgram`_ and `clBuildProgram`_ to enable/disable whole function
vectorization. These choices are described in detail in the extension
specification.

A new entry point ``clGetKernelWFVInfoCODEPLAY`` is added to allow whole
function vectorization information to be queried for a specified kernel, given
a device and local work size.

``clGetKernelWFVInfoCODEPLAY`` supports two parameter queries:

* ``CL_KERNEL_WFV_STATUS_CODEPLAY`` queries the status of whole function
  vectorization.
* ``CL_KERNEL_WFV_WIDTHS_CODEPLAY`` queries the list of widths for each
  work-item dimension.

USM - ``cl_intel_unified_shared_memory``
----------------------------------------

Intel's :doc:`extension/cl_intel_unified_shared_memory` extension adds support
for Unified Shared Memory(USM) to OpenCL, where rather than ``cl_mem`` handles,
memory allocations are represented as pointers in the host application. Unified
Shared Memory additionally provides fine-grained control over placement and
accessibility of an allocation. This is a device extension rather than a
platform extension as every device in the OpenCL platform may not provided the
minimum level of support required to implement the extension.

Only host and device allocations are supported, **not** shared USM allocations
which are intended to migrate between the host and one or more devices. This is
because our goal is to support SYCL 2020 `Explicit USM`_ which does not require
this capability.

Extension requires `clSetKernelExecInfo`_ from the OpenCL version 2.0 API which
we don't implement as part of our default 1.2 implementation. However, we have
backported the entry point to 1.2 in extension ``cl_codeplay_kernel_exec_info``.
Therefore to support the USM behaviour provided by ``clSetKernelExecInfo`` in a
1.2 build the user can use ``clSetKernelExecInfoCODEPLAY`` with
``cl_codeplay_kernel_exec_info`` enabled. Alternatively an OpenCL version 3.0
build of the oneAPI Construction Kit can be used without our
``cl_codeplay_kernel_exec_info`` extension, where the entry point is available
as core to be optionally implemented.

The API and enum definitions for USM are part of ``CL/cl_ext.h`` in the
external OpenCL-Headers repository.

USM functionality is tested in UnitCL by the tests found in the directory
``source/cl_intel_unified_shared_memory`` of UnitCL, which can be run with
``--gtest_filter=*USM*`` or build target ``ninja check-UnitCL-USM``.

Support for ``clHostMemAllocINTEL()`` is an optional feature of the extension
which may be queried for using ``CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL``.
Currently we only support host USM allocations if a ComputeMux device can use
host allocated memory by reporting ``mux_allocation_capabilities_cached_host``
and the ComputeMux device has the same pointer size as host.

.. _Explicit USM:
  https://github.com/intel/llvm/blob/sycl/sycl/doc/extensions/USM/USM.adoc#explicit-usm
.. _clSetKernelExecInfo:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clSetKernelExecInfo

SPIR-V USM Generic Storage Class - ``SPV_codeplay_usm_generic_storage_class``
-----------------------------------------------------------------------------

To support USM functionality in SYCL, ComputeCpp has found it necessary to
generate SPIR-V without address space information in it. To enable this the
:doc:`/modules/spirv-ll/extension/spv_codeplay_usm_generic_storage_class`
extension was created. With this extension enabled a SPIR-V module can pass the
``Generic`` storage class for all of its pointer type declarations to indicate
that no address space information is included in the declaration. Our SPIR-V
translator interprets all such pointer type declarations as having address
space 0. This address space was chosen primarily because this is the address
space ComputeCpp uses internally for this, and secondarily because it means
that function scope variables can remain as ``alloca``\ s, which is helpful for
later optimizations.

Note that this overrides the normal semantics of storage class ``Generic``. We
wrote the extension this way rather than adding a new storage class so we could
keep using existing SPIR-V tools without needing to fork them to add support.

Command-Buffers (Provisional) - ``cl_khr_command_buffer``
---------------------------------------------------------

oneAPI Construction Kit implements version 0.9.1 of the provisional
`cl_khr_command_buffer`_ extension.

The simultaneous-use optional capability is supported, which allows the
same command-buffer to be repeatedly enqueued without blocking in user
code.

`cl_khr_command_buffer`_ is intended as a base extension that other future
extensions will layer upon. These will provide additional features such as
recording a command-buffer across multiple command-queues, potentially
associated with different devices, as well as the ability to modify a
command-buffer recording in between replays.

Our implementation currently has the following limitations which need resolved
before the extension can be turned on by default in the oneAPI Construction Kit:

* Event profiling is not implemented (see CA-3322).

.. _cl_khr_command_buffer:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_command_buffer

Command Buffers: Mutable Dispatch - ``cl_khr_command_buffer_mutable_dispatch``
------------------------------------------------------------------------------

oneAPI Construction Kit implements version 0.9.0 of the provisional
`cl_khr_command_buffer_mutable_dispatch`_ extension.

`cl_khr_command_buffer_mutable_dispatch`_  builds upon `cl_khr_command_buffer`_
to allow users to modify kernel execution commands between enqueues of a
command-buffer.

.. _cl_khr_command_buffer_mutable_dispatch:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_command_buffer_mutable_dispatch

Extended Async Copies - ``cl_khr_extended_async_copies``
---------------------------------------------------------

oneAPI Construction Kit implements the `cl_khr_extended_async_copies`_ extension.

This has a default implementation which does simple copies of memory. This can be
replaced by a customer implementation where it can be accelerated.

.. _cl_khr_extended_async_copies:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html#cl_khr_extended_async_copies
