Mapping Custom IP Blocks To Builtin Kernels
===========================================

ComputeAorta allows targeting arbitrary fixed-function hardware (sometimes
called "IP blocks") through the use of builtin kernels. Rather than being
compiled from source code and executed by a programmable core, a builtin kernel
performs an operation implemented in hardware - for example, an accelerated
matrix multiplication.

Builtin kernels can be enqueued in the same way as programmable kernels, with
the user setting arguments at runtime. Fixed-function hardware will typically
provide better performance at the expense of programmability. By allowing both
types of kernels to be used in the same context, a user can take full advantage
of the hardware capabilities of the device without sacrificing programmability.

Builtin kernels are enqueued to command buffers in the same way as regular
kernels, allowing the ComputeMux runtime implementation to reorder the commands
and execute them in parallel (*as-if* they are in order) across IP blocks. For
example, a device which contains two separate IP blocks and a group of compute
cores may be able to execute two builtin kernels and a regular kernel
simultaneously, if the hardware supports it.

If the IP block supports multiple operations, each operation will usually be
exposed as a different builtin kernel.

A ComputeMux implementation **shall** provide the function prototypes of each
builtin kernel. The types used in the function prototype **must** be valid
OpenCL types.  ComputeAorta will then ensure these builtin functions can be
enumerated through the `clGetDeviceInfo`_ function in the OpenCL API.

A ComputeMux implementation **shall** provide an execution path for each
builtin kernel. Typically this will be a call to a low-level device driver that
can initiate the hardware operation with the arguments given to the kernel by
the user. ComputeAorta provides these arguments to the ComputeMux
implementation at runtime as part of the command buffer implementation.

If a builtin kernel has pointer arguments (that is, it takes a pointer to
shared memory), then this memory **must** be addressable by the hardware
operation being called, in the same way it **must** be addressable by a
software kernel. If, for example, the hardware operation instead performs on a
discrete area of memory that is only accessible by the IP block, the ComputeMux
implementation may instead implicitly copy the memory to and from this region
as necessary. If the memory is addressable but requires address translation,
this may also be performed at runtime by the ComputeMux implementation.

Multiple builtin kernel executions may appear in a single command buffer; the
ComputeMux implementation is free to perform kernel fusion style optimizations
on them if possible.

.. tip::
  Builtin kernels aren't limited to targeting operations implemented in
  hardware. For example, a library of hand-optimized assembly kernels may be
  exposed to the end-user via builtin kernels.

.. _clGetDeviceInfo:
  https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetDeviceInfo
