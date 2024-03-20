Host CPU
========

.. toctree::
  :maxdepth: 1

  mux/targets/host/extension/cl_codeplay_host_builtins
  mux/targets/host/extension/cl_codeplay_set_threads
  mux/targets/host/lit
  mux/targets/host/debug

Codeplay's reference ComputeMux target, for host (x86, Arm & Aarch64)
implementation.

Host Device
-----------

The information reported by a host device can vary depending on the build
configuration of the oneAPI Construction Kit. See the
[Developers Guide](developer-guide.md#oneapi-construction-kit-cmake-options) for
details on the effects of host specific CMake options.

Interesting Properties
^^^^^^^^^^^^^^^^^^^^^^

The Host implementation of ComputeMux can get away with various assumptions that
a ComputeMux implementation for an independent process cannot.

Assumptions possible for a "host" ComputeMux implementation:

* ``sizeof(void*)`` is the same on host and device.
* ``sizeof(size_t)`` is the same on host and device.
* All memory is shared and coherent between host and device.
* All address spaces map to the same physical memory.
* A single function pointer can be executed on host and device, i.e. can
  compile kernels directly to memory without needing to reload them.
* `Compiler-rt <https://github.com/llvm/llvm-project/tree/master/compiler-rt>`_
  type functions will be present without building them, e.g., ``__udivsi3`` or
  ``__floatdidf``.

Other properties for a "host" ComputeMux implementation:

* Running kernels takes CPU resources, so the user application, the runtime,
  and the kernels are all competing for resources.  E.g. an OpenCL application
  running on a GPU-style device may busy-wait to get low-latency, but for
  "host" that wastes a CPU that could be used for compute.
* Because "host" does not need to upload kernels to a separate memory it is
  conceptually easier to defer compilation until close to the point of
  execution.

Float Support
^^^^^^^^^^^^^

On 32-bit ARM builds we run both half and single precision floats on NEON
which has Flush To Zero(FTZ) behaviour. As a result the host device doesn't
report support for denormal numbers. This does not apply to double precision
floats where denormals are supported.

Compilation Options
^^^^^^^^^^^^^^^^^^^

On builds where both ``CA_ENABLE_DEBUG_SUPPORT`` is set and a compiler is
available the host device reports the following custom build options for the
``compilation_options`` member of ``mux_device_info_s``.

.. code:: console

  $ ./clc --help

  ComputeAorta x86_64 device specific options:
    --dummy-host-flag     no-op build flag
    --dummy-host-flag2    no-op build flag
    --dummy-host-option value
                          no-op option which takes a value

These are provided to test the mechanism for reporting and setting device
specific build options. The only effect they have on kernel compilation is
being propagated as program metadata, to assist testing so we can check options
have been correctly parsed. LLVM metadata node ``host.build_options`` is set to a
string matching the contents of ``compiler::Options::device_args``.

### Performance Counters

Support for counter type queries is implemented in host with ``PAPI``, a low
level performance counter API. This support can be enabled with the
``CA_HOST_ENABLE_PAPI_COUNTERS`` cmake option, and it requires that the PAPI
development libraries can be found on the system. PAPI can be built on Windows
but the way we measure on our worker threads is platform specific, so PAPI
performance counters are currently only supported on Linux. Before setting up a
query pool with a list of counters you want to measure, you should query the
available counters and pass the selection you want to use to the papi util
``papi_event_chooser``. This will tell you if your chosen events are compatible
with one another. Follow the links for `a general overview of PAPI
<https://bitbucket.org/icl/papi/wiki/PAPI-Overview.md>`_ and `detailed API
documentation <http://icl.cs.utk.edu/papi/docs/index.html>`_.

Host Binaries
-------------

Host can generate and accept binary executables, possibly containing multiple
kernels each. They use LLVM JIT's TargetMachine's object file format for
storing the executable code after optimizations and other LLVM passes, which is
then relinked into the running program using LLVM's RuntimeDyld dynamic linker
wrapper. The object file has an additional section called `.notes`, which stores 
information about the kernels such as their names and local memory usage.

``.notes`` section binary format
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``.notes`` section contains a valid Metadata API binary. You should use
the metadata API (documented :ref:`here <modules/metadata:metadata api>`)
to deserialize any metadata it contains. The metadata in this section is 
created by the :ref:`AddMetadataPass <modules/compiler/utils:addmetadatapass<analysisty, handlerty>>`.

Host Scheduled Kernel
---------------------

For host, scheduled kernels take a kernel residing within its own LLVM module,
and:

* Assert that the local work sizes for the ``x``, ``y`` & ``z`` dimensions are
  ``1``
* Clone the module into a new LLVM context owned by the host scheduled kernel
* Add declarations for work item functions that aren't used in the kernel, but
  will be called by other work item functions that are used
* Change the function signature for all function definitions to include a new
  work group information parameter
* Add definition for ``get_local_id`` and ``get_local_size`` based on local
  work sizes
* Add definition for ``get_global_size``, ``get_global_offset`` &
  ``get_group_id`` based on work group information parameter
* Add definition for ``get_global_id`` calculated from ``get_local_id`` and
  ``get_group_id``
* Create a packed struct of the parameter types for the kernel
* Add a new kernel wrapper function that takes the packed struct, unpacks each
  parameter and then calls the actual kernel
* Add a new wrapper function that loop over the ``x``, ``y`` & ``z`` global
  dimensions
* The ``x``, ``y`` & ``z`` values are set into the work group information
  parameter

LLVM Passes
-----------

.. _hostbimuxinfo:

HostBIMuxInfo
^^^^^^^^^^^^^

The host target subclasses ``BIMuxInfoConcept`` via ``HostBIMuxInfo`` to
override the list of scheduling parameters added to functions, used to lower
work-item builtins and add :ref:`work-group scheduling loops
<addentryhookpass>`.

The ``HostBIMuxInfo`` is used to add the three scheduling parameters detailed
below to kernel entry points and work-item builtins via the
:ref:`AddSchedulingParametersPass
<modules/compiler/utils:AddSchedulingParametersPass>`, is used to lower
work-item builtins in the :ref:`DefineMuxBuiltinsPass
<modules/compiler/utils:DefineMuxBuiltinsPass>`, and to initialize these custom
parameters in the :ref:`AddKernelWrapperPass
<modules/compiler/utils:AddKernelWrapperPass>`.

In addition to the :ref:`default work-item info struct
<modules/compiler/utils:Target Scheduling Parameters>`, the host target adds
two custom structures.

Schedule Info
^^^^^^^^^^^^^

The ``Mux_schedule_info_s`` structure (or "scheduling info" structure) is a
kernel ABI parameter for the host target. It must therefore be passed to the
kernel by the driver.

It is largely a copy of the defualt work-group info structure, but with two
additional parameters - ``slice`` and ``total_slices`` - to help construct the
:ref:`work-group scheduling loops <AddEntryHookPass>`.

.. code:: c

  struct Mux_schedule_info_s {
    size_t global_size[3];
    size_t global_offset[3];
    size_t local_size[3];
    size_t slice;
    size_t total_slices;
    uint32_t work_dim;
  };

Mini Work-Group Info
^^^^^^^^^^^^^^^^^^^^

Since many of the default work-group info fields are present in
``Mux_schedule_info_s``, the "mini work-group info" struct contains only the
group id and the number of groups.

.. code:: c

  struct MiniWGInfo {
    size_t group_id[3];
    size_t num_groups[3];
  };

This structure does not present itself as an ABI parameter. Its ``num_groups``
fields are initialized from calculations on ``Mux_schedule_info_s``, and its
``group_id`` fields are initialized by ``AddEntryHookPass`` by each level of
the work-group loops.

.. _addentryhookpass:

AddEntryHookPass
^^^^^^^^^^^^^^^^

`AddEntryHookPass`  performs work-group scheduling. The pass then adds
scheduling code inside a new kernel wrapper function which calls the previous
kernel entry function per work-group slice. A "work-group slice" is defined
here as a 3D set of work-groups over Z, Y, and a subset of the X dimensions
split evenly across the number of threads used by the host mux target.

This pass assumes that the :ref:`AddSchedulingParametersPass
<modules/compiler/utils:AddSchedulingParametersPass>` has been run, and that
the necessary scheduling parameters have been added to kernel entry points,
detailed :ref:`above <hostbimuxinfo>`.

Once (up to) three levels of work-group loops have been added, the `MiniWGInfo`
structure's `group_id` fields are updated by the scheduling code in each loop
level before the call to the original kernel.

AddFloatingPointControlPass
^^^^^^^^^^^^^^^^^^^^^^^^^^^

`AddFloatingPointControlPass` is a hardware aware pass which calls specialized
hardware helper functions to do work. If the targeted architecture isn't
supported then the pass exits early without modifying the module.

The pass is designed to set the hardware's floating point control register
in a wrapper before calling the kernel. Then afterwards restore the register
to it's original state before exiting.

An important configuration in the pass is setting FTZ (flush to zero) behaviour
for floating point denormal numbers. FTZ mode means that numbers smaller than
the minimum normal exponent, called denormals, are treated as `0.0`. Disabling
it gives us extra precision at the cost of performance.

Arm
***

In our host implementation this pass only affects double precision floats since
our single precision float operations are run on NEON which is FTZ by design
and cannot be changed. This is configured by using compiler flag `+neonfp` when
creating a host finalizer. Double precision floats are run on the VFP unit.

To disable FTZ we set the `CL_FP_DENORM` bit in the floating point status and
control register `fpscr` to zero. Implemented in the pass by using LLVM
intrinsic `arm_set_fpscr` to zero out the whole `fpscr`, disabling FTZ and
setting the rounding mode to 'round to nearest even'. Our pass also uses the
intrinsic `arm_get_fpscr` on kernel entry to grab the previous `fpscr` state so
we can restore it again on exit with `arm_set_fpscr`.

Aarch64
*******

In the aarch64 LLVM target there are no intrinsics for getting or setting the
FPCR register, and as a result we need to do it using inline assembly. This
involves linking the `LLVMAArch64AsmParser` library, which is extra overhead.
Therefore if intrinsics become available we should be reimplement this
functionality to use them instead of inline assembly.

Unlike Arm both single and double precision floating point instructions are
run on the same, and now unified, VFP/NEON unit.

X86
***

On x86 and x86_64 the MXCSR register is used to configure the behaviour of
the SSE instructions where we are running floating point computations.
