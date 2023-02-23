How To Support Large Scratchpad Memories
========================================

It is common for accelerators to have dedicated (e.g. on-chip/on-package) memory
that is usually much faster to access than DDR (system RAM) as well as much
smaller in size. This kind of memory is then used as a 'scratchpad' where chunks
or tiles of input data are streamed to this memory from DDR, potentially
modified by accelerator cores and streamed back to DDR. This differs from
approaches where accelerator cores read from and write to DDR directly, which
would likely be much slower due to DDR latency.

While dedicated accelerator memory comes in many configurations and serves
different purposes, in this section we focus on two different use cases:
`dedicated global memory`_ and `dedicated shared memory`_.

Dedicated Global Memory
-----------------------

With the first kind, kernels can use global buffers in just the same way as if
they were stored in DDR, with no or minimal changes to the kernel source.
Performance is improved not only due to smaller latencies but also by avoiding
having to transfer data between the CPU and the accelerator or perform cache
management operations between kernel invocations. Managing this kind of memory
is done on the host side, by `passing a special flag <https://github.com/codepla
ysoftware/standards-proposals/blob/master/onchip-memory/sycl-1.2.1/onchip-memory
.md#use_onchip_memory-property>`_ when creating buffers at the SYCL level or
using a hardware-specific entry point at the OpenCL level.

For this use case, the memory needs to have certain characteristics. It needs to
be accessible to all execution units in the same way (i.e. cores do not have
their own 'private' chunk of this memory) and persist across kernel invocations.
The memory does not need to be mapped in the CPU's address space but there needs
to be a mechanism such as DMA to transfer data from and to global (DDR) memory.
It would be desirable for memory transfers to happen concurrently with kernel
execution to allow double-buffering. The accelerator ISA would also provide load
and store instructions that can handle both DDR and scratchpad memory addresses.

Dedicated Shared Memory
-----------------------

With the second kind, memory is mapped to execution units in such a way that not
all units can access all of the memory. For example, units (cores) might be
grouped into clusters, each of which has its own slice of memory that is not
accessible to other clusters. Each core in the cluster then 'shares' the memory,
hence the name. This kind of memory resembles OpenCL and SYCL local memory,
however it might not completely align with that model. There might be multiple
clusters in a work-group (i.e. the memory might not be visible to the entire
work-group), and there might also be ways to trigger (DMA) transfers between
shared memory and DDR from kernels. By letting the accelerator manage the memory
transfers using in-kernel DMA transfers, performance is improved by avoiding
synchronization between CPU and accelerator.

For this use case requirements are not as strict and variations between
different hardware architectures are expected to be much greater than for the
'dedicated global' memory use case. This might take the form of OpenCL/SYCL
local memory with added in-kernel DMA builtin functions, a new memory address
space or even memory that is generally sliced by execution cluster and accessed
directly by cores in the cluster but with special DMA operations that allow
transfers between clusters. In general, the expectation is that this kind of
memory behaves in a more special way from the user's perspective compared to the
first use case.

Implementation Notes
--------------------

With both approaches it might not be possible to expose scratchpad memory using
existing concepts in the OpenCL/SYCL memory model, and it may be necessary to
tag in-kernel pointers in such a way that they are separate from global and
local pointers. This might be due to a standard address space being backed by
different kinds of memory (e.g. DDR or scratchpad in the dedicated global memory
use case) and wanting to expose hardware-specific features that can only be used
with scratchpad memory. In this case a new (custom) address space needs to be
implemented in the compiler front-end (e.g. by adding a new language keyword to
extend the OpenCL C language) as well as other parts of the compiler. Special
handling of kernel arguments with this new address space may be needed in the
runtime (ComputeMux target). See :ref:`overview/compiler/ir:Address Spaces` for
more details on how address spaces are handled in the compiler.

While it might be easier for the user if dedicated memories are exposed using
standard compute concepts that fit the OpenCL/SYCL model, hardware
idiosyncrasies and features sometimes require a proprietary extension to the
standard in order to fully take advantage of these hardware-specific features.
With ComputeAorta this involves creating a new OpenCL extension, which could
introduce new entry point functions to the OpenCL host as well as new builtin
functions that can be called by OpenCL C kernels. Exposing hardware-specific
functionality through builtin functions is described more in detail in the
:doc:`/overview/example-scenarios/mapping-algorithms-to-vector-hardware`
section.
