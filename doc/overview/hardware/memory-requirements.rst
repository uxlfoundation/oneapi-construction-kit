Memory Requirements
===================

Mapping ComputeMux memory requirements to hardware in an optimal way depends on
the type of hardware. A multi-core CPU has different memory access
characteristics than a DSP or AI accelerator which again differ from a discrete
GPU.

ComputeMux inherits the concept of an `address space <Address Spaces>`_ (memory
region) from OpenCL. In this section we will begin with some examples of
hardware and walk through how various types of physical memory are mapped to
address spaces.

For now, let us focus on a hypothetical DSP.

Example DSP Memory Mapping
--------------------------

Our hypothetical DSP has a memory topology with a register file, on-chip tile
memory, and access to DDR.

DDR is mapped to the global_ address space. Cache coherency, or even a cache,
is not required. Synchronization of data occurs at explicit points before and
after kernel execution, or during kernel execution via DMA operations enabled
by support for `Pinned Memory`_.

.. tip::
   Through extensions it is also possible to expose additional data
   synchronization mechanisms to enable the user to extract the maximum
   performance from the hardware.

On-chip tile memory is mapped to the local_ address space. Data transfers into
and out of on-chip tile memory can be invoked during kernel execution
asynchronously, allowing for efficient tiled processing of one set of data
while another is being loaded or stored.

.. seealso::
   :doc:`/overview/example-scenarios/how-to-support-large-scratchpad-memories`
   for some more in depth scenarios of utilizing on-chip memory effectively.

The register file is mapped to the private_ address space, this is where the
kernel's live variables are stored. In the event that a kernel exhausts the
register file capacity, a portion of on-chip tile memory should be reserved for
the private_ address space for use as stack space.

.. TODO: Example Multi-core CPU
.. ----------------------

.. TODO: Example AI Accelerator
.. ----------------------

.. TODO: Example Discrete GPU
.. --------------------

.. TODO: Example FGPA
.. ------------

Address Spaces
--------------

The OpenCL specification section on `Fundamental Memory Regions`_ describes
four named address spaces for device memory.

global
......

   A memory region read/write accessible to all work-items executing a kernel
   instance on device.

The global address space is mapped to the device's DRAM when available or
system RAM otherwise.

.. seealso::
   The driver requirements section on
   :ref:`overview/runtime/driver-requirements:Memory Management` for more
   information about how memory in the global address space is managed at
   runtime.

constant
........

   A region of global memory read accessible to all work-items executing a
   kernel instance on device.

The mapping of the constant address space depends a lot on the hardware. If the
hardware has a suitable memory region, such as GPU uniform memory, the constant
address space will be mapped there. In most cases the constant address space
will be mapped in the same way as the global address space.

local
.....

   A memory region shared by work-items in a work-group, **not** accessible to
   work-items outside of work-group. Variables allocated in the local address
   space inside a kernel function are allocated for each work-group executing
   the kernel and exist only for the lifetime of the work-group executing the
   kernel.

Commonly mapped to on-chip memory attached to a single processor core designed
to efficiently store a tile of temporary data. OpenCL provides asynchronous DMA
builtin functions for moving data between the local and global memory address
spaces that capable hardware can make use of.

private
.......

   A region of memory private to a work-item. Variables defined in a work-item's
   private memory is not visible to another work-item.

Memory private to individual work-items is usually mapped to registers or stack
space on a processor core.

.. figure:: /_static/images/opencl/memory_regions.png

  OpenCL Memory Regions - `Copyright The Khronos Group CC BY 4.0`_

.. _Copyright The Khronos Group CC BY 4.0:
   https://github.com/KhronosGroup/OpenCL-Docs/blob/master/copyrights-ccby.txt
.. _Fundamental Memory Regions:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_fundamental_memory_regions

Pinned Memory
-------------

An operating system which manages virtual memory is not required to guarantee
that virtually contiguous pages are physically contiguous, and can move
physical pages or opt to evict them to a swap partition in some circumstances.
This is problematic when attempting to share data with another processor which
has physical access to DDR, such as a DSP or discrete GPU.

Fortunately, it is possible to tell the operating system to *lock* or *pin*
physically contiguous regions of memory to ensure they will not be moved or
evicted. We refer to this as pinned memory.

Pinned memory can be an important mechanism to enable optimization of data
movement. Given a memory address, we can query the operating system to find out
if it resides in a pinned memory region. Using this information, a device
driver can determine if it is possible to optimize data transfers using DMA
operations, or a slower fallback which can handle page faults is necessary.

.. important::
   When a device does not have dedicated memory, pinned memory becomes a
   requirement for accessing system RAM mapped to the global_ address space.

.. TODO: DMA to/from global memory
.. TODO: USM
