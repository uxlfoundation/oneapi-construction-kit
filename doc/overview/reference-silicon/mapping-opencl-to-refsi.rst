How The Platform Maps to SYCL and OpenCL
========================================

Memory Mapping
--------------

Example memory map for accelerator (RISC-V) cores:

* 000001000-0001fffff: [shared, window] Executable memory (2 MB) kernel(s)
* 010000000-0103fffff: [per-core] Core TCDM (4 MB) private memory (inc. stack),
  local memory, scheduling info
* 018000000-0187fffff: [shared, window] Combined TCDM (8 MB)
* 020002000-0200020ff: [per-hart] DMA management registers
* 040000000-13fffffff: [shared] DRAM (4 GB) global memory

In the memory map above, 'shared' means that the contents of the memory area are
the same no matter which core accesses it, 'per-core'/'per-hart' that the
contents are different depending on which core/hart accesses the memory.

TCDM is listed twice in the map, with one entry marked 'core' and the other
'combined'. This is the same memory, but mapped twice in different ways. The
'combined' area is meant to be used for DMA while the 'core' area is the private
view of a core.

Combined memory is laid out so that the first core's TCDM chunk is present
first, followed by the second core's and so forth. An alternative layout could
be to interleave cores' TCDM chunks with a fixed increment (e.g. 4 KB). This
would make it easier to use TCDM in both 'core' and 'combined' modes at the same
time.

Since TCDM is both fast and per-core, it is ideal for storing the stack. For the
same reasons it could be used for OpenCL/SYCL local memory.

DRAM, being the largest pool of memory directly accessible to accelerator cores,
could be used for OpenCL/SYCL global memory. Device-level Unified Shared Memory
support should be implemented for the RefSi M1 platform.

The number of platform (e.g. CMP) registers will be limited to 256 64-bit
registers.

How To Map Work on RefSi M1
---------------------------

The mapping of work groups to execution units in the accelerator (harts) that is
predicted to have the best performance on this virtual platform is the
'work-group per thread' approach. With this approach, each RISC-V hart in the
accelerator independently executes a separate work-group. Since there are 8
harts, 8 work-groups can be executed in parallel. The RVV extensible vector
extension from RISC-V is used to execute work-items inside the work-group in
SIMD fashion to maximize integer and FP throughput provided by the RVV vector
units.

The CMP is responsible for scheduling a series of kernel function invocations
(one per 'instance') on the accelerator cores when a 'Run Kernel Slice' command
is processed. This involves setting up all the necessary registers such as the
stack pointer, the program counter (set to the kernel entry point in the ELF
executable) and passing programming-supplied arguments to the function. Each
hart is resumed by the CMP to execute the kernel function once and becomes idle
again once the function finishes. Depending on how many instances are in the
slice and the chosen scheduling approach, a given hart can go through several
active/idle cycles before the kernel has finished executing.
