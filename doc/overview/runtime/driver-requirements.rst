Driver Requirements
===================

ComputeAorta requirements for the hardware device driver used to implement a
:ref:`Custom ComputeMux Runtime
<overview/introduction/architecture:custom runtime>`. Driver requirements are
organized into the following categories:

Must-Have
  Driver requirements which are mandated by ComputeAorta.

Should-Have
  Possible to support ComputeAorta without this requirement, but may lead
  to limited functionality, degraded performance, or additional complexity in
  implementation.

Nice-to-Have
  Will make the hardware easier to develop for, speeding up delivery time
  and/or improving performance.

Memory Management
-----------------

Requirements of the device driver in regards to memory handling.

Distinct Address Space
~~~~~~~~~~~~~~~~~~~~~~

The following is required of the driver when host and device do not share a
unified address space. For example, a host CPU **may** see virtual addresses
mapped via a MMU with pointer values that differ to those on the device for the
same physical address.

.. important::
  ComputeMux **does not** require cache coherency between host and device when
  they have access to shared memory. The implementation of ComputeMux commands
  for reading and writing memory buffers **may** explicitly flush caches via
  driver calls to manually maintain coherence.

.. rubric:: Must-Have

* Allocate/deallocate device accessible pinned memory with alignment control.
* Synchronize regions of memory between the host and device. For a host and
  device that have access to a shared memory but different caches, this
  amounts to cache flushing.
* Copy data between memory regions. This is assumed to be implemented via a DMA
  operation that does not require host interaction. Not required if
  device-accessible memory regions can be mapped to the host-accessible memory
  address space.
* Copy data between host-accessible memory and device-accessible memory
  regions. This is assumed to be implemented via a DMA operation that does not
  require host interaction. Not required if device-accessible memory regions
  can be mapped to the host-accessible memory address space.

.. rubric:: Should-Have

* Fill data from a pattern into memory (sub-)regions. Not required if
  device-accessible memory (sub-)regions can be mapped to the host-accessible
  memory address space.
* Define intended memory usage when allocating device memory regions,
  e.g. read/write/execution access by the device and read/write/no access by the
  host.
* Define data usage when mapping/synchronizing (sub-)regions of memory between
  the host and device e.g. synchronizing for read/write/invalidate-and-write
  accesses.
* Map device-accessible memory (sub-)regions to the host-accessible memory space.

.. rubric:: Nice-to-Have

* Allow allocations to pin memory that is mapped from the device to host memory
  address space, to a user-provided host memory address.
* Tag usage of device memory regions and sub-regions as read/write/execute
  access.

Unified Address Space
~~~~~~~~~~~~~~~~~~~~~

If host and device share the same unified address space, i.e. pointers on host
map to the same physical addresses as equivalent pointers on the device, then
the driver needs to provide functionality to meet the following requirements.

.. rubric:: Must-Have

* Trigger data synchronization between host and device to ensure memory
  consistency e.g. host to device and device to host cache flushing when host
  and device have separate caches.
* Allocate and deallocate unified memory.

.. rubric:: Should-Have

* Copy data to/from device memory or caches via DMA while the device executes a
  kernel.
* Inform about DMA data move completion, possibly integrated with
  `Command Scheduling`_.

.. rubric:: Nice-to-Have

* Tag usage of memory regions and sub-regions as read/write/execute access by
  the device.

Command Scheduling
------------------

Heterogeneous APIs add commands to a device specific queue, which can be
asynchronously flushed so that the commands are executed on the device while
the host application continues with other work. Commands could be executing
kernels, or memory operations performed on buffers such as a fill or copy.
ComputeAorta internally batches commands which can be executed together without
external dependencies into groups sent to the ComputeMux Runtime, and then onto
the driver.

Batches of commands without dependencies between them can be executed in
parallel on different hardware units, as can the commands within a individual
batch - provided the correct dependency analysis has been done between
operations. This means there is a graph of dependencies which a Mux runtime
implementation can schedule across multiple execution units on the device,
without needing to copy data to and from host or return control to host between
operations.

Requirements of the driver to allow ComputeAorta to submit commands
are the following:

.. rubric:: Must-Have

* Synchronous scheduling of commands to the device, however this is not optimal
  and asynchronous scheduling is preferred.
* Load a kernel binary to the device and launch the executable code.
* Inform about command state changes. Minimum states are:

  * Ready - Command submitted but not yet executing.
  * Running - Command is currently executing.
  * Complete - Command has finished and all data changes are visible to the next
    command.

.. rubric:: Should-Have

* Asynchronous scheduling of commands to the device, so that busy-waiting is not
  required and the driver informs ComputeAorta about completion. This driver
  functionality is used to implement semaphore objects in the ComputeMux Runtime
  API.
* Ability to specify an entry-point when launching executable code, removing
  start-up overhead when running an individual kernel from a binary containing
  multiple kernels.
* Inform about command state changes for the additional states on top of the
  minimum:

   * Finished execution - Command has completed but data changes not yet visible
     to the next command.
   * Failed - Command has failed to complete.

* Inform about command termination reasons, e.g. memory access violations.
* Combine commands with associated data transfers, e.g. DMA transfers. Without
  this it is hard to coordinate efficiently, as commands cannot be dispatched
  before their associated DMA transfers have finished.
* Handle dependencies within a graph of commands issued to the device,
  including terminating all commands depending on a failed command.
* Direct control of the core that commands are scheduled on.

.. rubric:: Nice-to-Have

* Collect timing information for command state changes to be used in profiling.
* Ability to cancel a command execution that is in progress, allowing for
  cleaner teardown when commands timeout or deadlock.
* Facility to set callbacks invoked by the driver on change of command state.
* Provide an interface to access any non-programmable fixed function hardware
  which the customer would like users to have access to. The ComputeMux Runtime
  implementation will expose these fixed function units to a user in OpenCL as
  `builtin kernels`_.


.. _builtin kernels:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clCreateProgramWithBuiltInKernels

Kernel Execution
~~~~~~~~~~~~~~~~

Running a ComputeMux kernel typically involves ComputeAorta interacting with
the device driver in the following steps:

1) Loading and relocating the kernel instruction stream to the targeted device
   core.
2) Setting up and loading kernel runtime data, such as work-group related
   information.
3) Loading, or at least partially caching, data to device memory.
4) Triggering executable code.
5) If data is accessed by the host, copy data from device to host memory
   after the kernel finishes.

.. rubric:: Must-Have

* Driver **must** allow the above steps to be performed synchronously.

.. rubric:: Should-Have

* Driver **should** enable the above steps to be performed asynchronously
  while another kernel is executing.

.. rubric:: Nice-to-Have

*  Driver provides profiling information about kernel execution, such as
   hardware clocks or cycle counters.

Error Handling
--------------

Requirements of the device driver so that ComputeAorta can recover from a
hardware failure without having to power cycle hardware.

.. rubric:: Should-Have

* The driver allows re-initialization of the device after failure without
  requiring a manual reboot of the hardware. This means the process which uses
  the driver can be terminated and then safely restarted.

  .. note::
    The capability to reset the host CPU is not required from the driver.

.. rubric:: Nice-to-Have

* Ability to reset individual parts of the device, e.g. specific accelerator
  components of a SoC.

Multi-Process Guarantees
------------------------

In an environment where multiple processes may be created that each use the
device driver, it is important there are guarantees about driver usage so that
resources are managed across the process instances.

.. important::
  If OpenCL will be used on an RTOS, then the driver needs to be compatible with
  the intended RTOS, which may run all applications under a single process.

ComputeAorta supports two possible models of multi-process enablement.

Spatial Partitioning
  The device execution units are physically partitioned and different processes
  run in each partition, useful in virtualized environments. The partitioning is
  likely to be statically configured at start-up, once.

Temporal Partitioning
  Multiple processes may be run on the same execution unit, but not at the same
  time, there is an operating system scheduler with the responsibility of
  scheduling processes.

To ensure that the drivers actions in one process do not influence another, the
driver should be compatible with either **Spatial Partitioning** or **Temporal
Partitioning**, so that it has knowledge about which process is making a request
and therefore which resources are available to it.

.. rubric:: Should-Have

* Freedom from interference of memory, device memory is partitioned and a
  process using one partition should be blocked from accessing memory outside of
  that partition.
* Freedom from interference of timing and execution, one process accessing the
  device should not stop another from executing or lead to deadlock, although
  one process occupying the device may cause a second process requiring
  resources of the device to delay until they become available.

Device Queries
--------------

ComputeAorta needs to report information about device resources to the final
application user. For this purpose, ComputeAorta has the following requirements
for queries that can be made to the driver.

.. important::
  When only a single process may access the device the query values can be
  hard-coded and there are no requirements on the driver. However, for the
  multi-process case where resources are shared, ComputeAorta will have to be
  able to be query the device driver.

.. seealso::
  For the OpenCL API, queries are used to implement the function
  `clGetDeviceInfo`_.

.. rubric:: Must-Have

* Number of cores.
* Number of vector units.
* Number of scalar units.
* Whether the device shares a unified memory address space with the host or not.
* Device global memory size
* Device constant memory size
* Device local memory size.

.. seealso::
  See :doc:`/overview/hardware/memory-requirements` for more information on the
  different types of device memory.

.. rubric:: Should-Have

* Driver version.
* Hardware version.
* Maximum clock frequency.
* Cache sizes, including cache line size and number of cache levels.

.. rubric:: Nice-to-Have

* Timer resolution for profiling.
* Type of device cache.

.. _clGetDeviceInfo:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetDeviceInfo

Images
------

This document does not cover the driver requirement to expose image hardware,
such as texture memory and shaders, as ComputeMux images. Details of the
ComputeMux runtime APIs that must be implemented to support images in higher
level open standards can be found in the :ref:`Images
<specifications/mux-runtime-spec:Images>` section of the Mux runtime
specification. `OpenCL images`_ are an optional feature of the OpenCL
specification that is usually omitted in preference for operating on buffers
containing the image data.

.. _OpenCL images:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_image_objects


Driver Delivery
---------------

Requirements for integrating the device driver libraries into ComputeAorta and
updating ComputeAorta to new releases of the driver.

.. note::
   This section is only a requirement when Codeplay is integrating a customer
   driver as part of a ComputeMux implementation. If a customer is integrating
   their own driver, this section can be ignored.

.. rubric:: Must-Have

* If the driver is delivered to Codeplay as source, it **must** be written in
  conformant C/C++ and build with a compiler that is available to Codeplay. If
  the compiler is not publicly available, it **must** be made accessible to
  Codeplay at the same time the driver is delivered.
* Every driver delivered to Codeplay has been thoroughly tested by the customer
  before release and there are no feature regressions between drops.

.. rubric:: Should-Have

* A single set of unified libraries for all target platforms, hardware and
  simulator, to avoid introducing complexity to ComputeAorta.

  .. important::
    Failure to provide unified libraries can lead to the situation where the
    ComputeMux Runtime needs to be implemented twice due to the divergence of
    hardware and simulator interfaces. This has significant implications for the
    complexity of the project and is strongly advised to be avoided, but is not
    **Must-Have** since engineering a solution is still possible.

Documentation
-------------

Requirements for documentation of the device driver.

.. note::
   This section is only a requirement when Codeplay is integrating a customer
   driver as part of a ComputeMux implementation. If a customer is integrating
   their own driver, this section can be ignored.

.. rubric:: Must-Have

* Documentation of driver API.
* Documented changes to APIs or behaviour between driver releases, ideally in
  the form of a changelog.
* If the driver is delivered to Codeplay as source, build instructions **must**
  be provided which can be reproduced on a fresh install of the target build
  operating system.

.. rubric:: Should-Have

* Documentation of limitations to regions of memory which are available, how
  memory can be allocated, and how memory can be flushed to and from the
  device.
* Documentation of limitations in the hardware or simulator platforms, as well
  as any divergence in capabilities or usage between the hardware and simulator
  platforms.

.. rubric:: Nice-to-Have

* Mechanisms for logging debug information about the driver's execution.
* Mechanisms for retrieving profiling information from the driver.
