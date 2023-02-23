# How OpenCL Concepts Are Mapped Onto ComputeMux

This document aims to give a description of how the ComputeAorta implementation
of [OpenCL 1.2][cl-12] maps down to our [ComputeMux Runtime][mux-spec] API and
[ComputeMux Compiler][compiler].

## Programs and Kernels

This section describes how the various compilation steps of OpenCL programs and
kernels interact with Mux.

### OpenCL Compilation Steps

Programs in OpenCL are represented with the `cl_program` object that supports
both programs created from source and are compiled at runtime, or programs
created from binaries on disk and are loaded directly by the OpenCL driver,
known as offline programs.

Runtime compilation in OpenCL starts with a `cl_program` that can be created
either from OpenCL C source using `clCreateProgramWithSource()`; from the
[SPIR][spir-12] binary format, via `clCreateProgramWithBinary()`; or from an
intermediate representation, such as [SPIR-V][spirv], via
`clCreateProgramWithIL()` or `clCreateProgramWithIRKHR()`. This `cl_program`
should then be built with `clBuildProgram()`, or by invoking
`clCompileProgram()` followed by `clLinkProgram()`. Once the program has been
built the user can create `cl_kernel`s to run on device using `clCreateKernel()`
with the name of the kernel function inside the program.

A `cl_program` can also be created directly from a pre-compiled binary using
`clCreateProgramWithBinary()`. In this path, the compiler is not utilised and
instead the binary is passed directly to the ComputeMux runtime.

When running the kernel, calls to `clEnqueueNDRangeKernel()` or
`clEnqueueTask()` are made contingent on the desired work dimensions.
ComputeAorta may still perform compilation late at these entry points before
running if the `cl_program` was created from source, depending on how the Mux
target is implemented.

### Runtime Compilation

The compilation process of a compiled `cl_program` is driven by the
`compiler::Module` object. These are created during the call to
`clBuildProgram()` and `clCompileProgram()`, which accept the source (either
OpenCL C, SPIR or SPIR-V) and turn it into IR by calling one of the
`compiler::Module::compile*` methods and/or `compiler::Module::link`, followed
by `compiler::Module::finalize`. 

When an OpenCL kernel is created, a corresponding `compiler::Kernel` object is
created, which acts as a reference to a kernel entry point inside the
`compiler::Module`. During `clEnqueueNDRangeKernel()` or `clEnqueueTask()`, the
needed workgroup configuration is provided to
`compiler::Kernel::createSpecializedKernel` which returns a `mux_kernel_t` and
(optional) `mux_executable_t` that can be passed to `muxCommandNDRange`.

It is up to the implementation of the `compiler::Module` and `compiler::Kernel`
interfaces as to whether backend passes and code generation is performed immediately
during `compiler::Module::finalize`, or deferred to `clEnqueue*` time
during `compiler::Kernel::createSpecializedKernel`.

### Offline Compilation

If the `cl_program` is created with a pre-compiled binary using
`clCreateProgramWithBinary`, then the `compiler::Module` class is not used.
Instead the binary is passed directly to Mux using `muxCreateExecutable`. 

The OpenCL `clCreateKernel()` call corresponds directly to `muxCreateKernel()`
through the compiler module, mapping the `cl_program` argument to a
`mux_executable_t`. Giving us a `mux_kernel_t` which is equivalent to a
`cl_kernel`. We cache this `mux_kernel_t` before returning to user code, so
subsequent calls to `clCreateKernel()` with identical parameters will return the
cached kernel rather than going through `muxCreateKernel()` again.


## Queues and Synchronization

In this section we detail how OpenCL queues and synchronization between commands
works on top of the Mux API.

### OpenCL Queues and Events

In OpenCL `cl_command_queue` queues are created for a specific device via
`clCreateCommandQueue()` and can have a variety of operations, called commands,
pushed to them for execution using APIs beginning with `clEnqueue*`.
By default commands are executed in-order with respect to a single command
queue, but to synchronize objects across multiple command queues `cl_event`
objects are needed.

`cl_event` objects are returned as an output parameter from a variety of calls
which can be waited on for completion using `clWaitForEvents()`. Alternatively
many enqueue APIs take an event list parameter of events that need to complete
before the command is executed. Commands are not immediately sent to the device
for execution, this only occurs on a flush. Flushes are triggered by calls to
`clFlush` and by blocking operations, such as `clFinish()`, and commands which
have a blocking `cl_bool` parameter set to `CL_TRUE`. Blocking operations
perform an implicit flush of the command queue followed by a wait until those
commands have completed.

### Mux Queues and Semaphores

ComputeMux represents queues using `mux_queue_t` objects which are tied to a
target device. Available queues for each device can be queried with
`muxGetQueue()` passing a bitfield `mux_queue_type_e` with desired properties
for the returned the queue. This is modelled after [Vulkan][vk] where different
queue types are important for the graphics pipeline. OpenCL however always maps
down to compute enabled Mux queues.

Collections of operations called `mux_command_buffer_t` objects are executed on
a queue using `muxDispatch()`. Before then, the command buffer must be created
with `muxCreateCommandBuffer()`, and operations are pushed to it with
`muxCommand<Command>` API calls. Examples include `muxCommandWriteBuffer()` for
writing data to a buffer, and `muxCommandNDRange()` to execute a specialized
kernel. The ordering in which commands are pushed to a command buffer is
guaranteed to be the order in which those commands are executed. Where possible
multiple calls to `clEnqueue*` entry points are pushed to a single command
buffer. These commands are then dispatched to the device when `clFlush` is
called, or an implicit flush occurs. Use of `cl_events` created by calls to
`clCreateUserEvent` cause command buffers which must wait on the user event to
be postponed until `clSetUserEventStatus` is called with the `CL_COMPLETE`
status.

OpenCL's `clFinish()` is implemented using the Mux API `muxWaitAll()` which
takes a queue and blocks until all command buffers dispatched to it have
completed. For more fine grained synchronization Mux uses `mux_semaphore_t`
semaphores passed to `muxDispatch()`. Here semaphores can be waited on before
the command buffer can begin execution, and set to signal once the command buffer
has completed.

In ComputeAorta each OpenCL command queue assigns a semaphore to each Mux
command buffer, which - when it is dispatched will be signalled once it completes.
Each command buffer also waits on the signal semaphore of most recently
dispatched command buffer, if one exists - thereby chaining command buffers
together in the same queue. The result is an in-order command queue. This is
important because it means that if an earlier command buffer in the queue is
blocked on progress of another command buffer, later commands in the same queue
won't jump ahead.

As well as the semaphore parameters `muxDispatch()` also takes a function
pointer callback argument which is invoked on completion. This callback is used
internally by our runtime to cleanup allocated resources, but also as part of
implementing `cl_event` OpenCL objects. Inside this callback we set the output
event associated with the command buffer to `CL_COMPLETE`. In turn this event may
then call its own callback which has been set by the user with
`clSetEventCallback()`, or internally by our runtime.

When an OpenCL call is enqueued with an event wait list for synchronization our
runtime does analysis of the events to determine if the enqueued command can be
pushed onto the current command buffer to be dispatched in a single batch. The
following is a list of cases which this analyses is looking for:

1.  There are no wait events and the last pending dispatch is not
    associated with a user level command buffer, get the current command
    buffer or the last pending dispatch.
2.  There are no wait events and the last pending dispatch is associated
    with a user level command buffer, get an unsed (new or reseted and cached)
    command buffer.
3.  There are wait events associated with a single pending dispatch, get
    the associated command buffer.
4.  There are wait events associated with multiple pending dispatches, get
    an unused (new or reseted and cached) command buffer.
5.  There are wait events with no associated pending dispatches (already
    dispatched), get an unused (new or reseted and cached) command buffer.

Additionally, when it is not possible to add a command to a command buffer,
semaphores are used to enforce the order in which command buffers are enqueued.
When a command buffer is passed to `muxDispatch()` to begin work, it will also be
provided with a list of semaphores to wait on in the following cases:

1.  There are no wait events and there is a running command buffer.
2.  There are no wait events and the last pending dispatch is associated
    with a user level command buffer.
3.  There are wait events associated with multiple pending command buffers
    dispatches.
4.  There are wait events, no pending dispatches, and there is a running
    command buffer.

All command buffer dispatches also provide a signal semaphore, which is tracked
by the OpenCL command queue, so that enqueue ordering can be maintained between
previously dispatched commands and new user commands.

:::{note}
The OpenCL layer resets and reuses the signal semaphore of completed command
groups. Therefore, command buffer implementations **must not** access Mux
semaphores of completed command buffers they wait on, otherwise command buffer
processing **may** deadlock.
See the Mux specification for command buffers.
:::

## Memory Objects

This section documents how memory objects and their various allocation
configurations is translated from OpenCL to Mux.

### OpenCL Buffer and Images

Memory objects in OpenCL are represented by the `cl_mem` object for both
buffers and images. Different allocation and usage information can be specified
when creating these objects via the `cl_mem_flags` parameter to
`clCreateBuffer()` and `clCreateImage()`. Such as whether the memory is read or
write only, and if the user code or runtime should allocate the memory.

Once a `cl_mem` is allocated it can be read, written, copied, and mapped by
enqueuing the operation to a command queue.

### Mux Memory

When user code calls `clCreateBuffer()` or `clCreateImage()` one of the first
things our OpenCL implementation does is issue a similar call through Mux,
`muxCreateBuffer()` or `muxCreateImage()`. These functions take a host-side
allocator argument `mux_allocator_info_t`, which is held by the device and
defines how memory used by the host should be allocated and freed.

However these Mux create APIs do not allocate physical device memory to store
the underlying data. Instead they act as views on regions of memory owned by a
`mux_memory_t` object. To allocate memory `muxAllocateMemory()` must be used
which returns a `mux_memory_t` object. This `mux_memory_t` is then bound to
the `mux_buffer_t` or `mux_image_t` by using `muxBindBufferMemory()` or
`muxBindImageMemory()`. These Mux create, allocate, and bind steps are all
performed by the OpenCL runtime for a `clCreate[Buffer,Image]` call.

The different allocation properties and memory coherence information that OpenCL
specifies in `cl_mem_flags` are mapped to two enums `mux_allocation_type_e` and
`mux_memory_property_e` which are passed to `muxAllocateMemory()`.

[cl-12]: https://www.khronos.org/registry/cl/specs/opencl-1.2.pdf
[mux-spec]: /modules/mux/runtime-spec.md
[compiler]: /modules/mux/compiler.rst
[spir-12]: https://www.khronos.org/registry/spir/specs/spir_spec-1.2.pdf
[vk]: https://www.khronos.org/registry/vulkan/specs/1.1/pdf/vkspec.pdf
