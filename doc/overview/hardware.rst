Mapping ComputeMux to Hardware
==============================

ComputeMux is a common API which allows multiple APIs such as *OpenCL* to map to
particular hardware devices. In such cases there is considered to be a *host*,
typically a *CPU*, and a *device*. *device* is usually multiple programmable
cores with a common fast memory. The *device* is expected to execute kernels
which are compiled through this API. This execution of kernels offloads work
from the *host*. The *device* or *host* can be multi-core and utilize that
aspect.

There is a *runtime* aspect to this that allows management of commands, which
normally part of the *host*. This allows creation of batches of commands which
**must** be executed on the *device*. These batches can also be seen as graphs
of commands based on data dependencies. These batches **may** be run entirely on
the *device*.

Hardware interfaces are expected to support:

- Allocation of memory on the *device* (the memory allocations are known as
  *buffers*)
- Reading and writing to memory in the *device* from the *host* (although this
  should be infrequent, usually large and can be via DMA)
- Executing programs (known as *kernels*) on the *device* using previously
  allocated buffers.

The execution of the *kernel* is expected to do the same work across different
data inputs and **may** run in parallel. This can map to hardware in different ways.
Conceptually work is split across a potentially large number of *work items* and
each *kernel* is defined at the *work item* level.

Each *work item* has a unique identifier and is expected to be executed as part
of a *work group*, each item working effectively in parallel, with some
synchronization if needed. A *work group* typically would run on a single core.
Work groups are expected to run asynchronously from each other. This is
described in more detail :ref:`here
<overview/example-scenarios/mapping-algorithms-to-vector-hardware:Work-group
Scheduling>`.

For an understanding of mapping device memory to ComputeMux concepts, see
:doc:`here</overview/hardware/memory-requirements>`

The other major part of ComputeMux is the compiler aspect. This sits on top of
an *LLVM* backend. The kernels themselves either are produced by the clang
*library* based on source kernel code such as OpenCL or come from lower level
intermediate languages such as *SPIR-V* which has a good mapping to and from
*LLVM*. From the ComputeMux target perspective an *LLVM* module will be used as
an input.

The ComputeMux target specific part can provide a number of passes to prepare
the kernels in the module for running on the target architecture, before it
passes it to the compiler backend to produce some form of object code. This
object code may have further processing to form some form of executable. The
executable **shall** be passed to the *device*, probably through a driver
interface. There a number of utility passes which are provided to assist in
writing this part.

Whether the final executable targets the work item level or the work group
level depends on the hardware and is described in more detail :ref:`here
<overview/example-scenarios/mapping-algorithms-to-vector-hardware:Work-group
Scheduling>`. Additional information such as scheduling will likely be passed
into the function as well as the kernel arguments.

More detailed requirements, including memory, atomics, floating point and
synchronization are detailed in the sections below.

.. toctree::

   hardware/memory-requirements
   hardware/atomic-requirements
   hardware/floating-point-requirements
   hardware/synchronization-requirements

