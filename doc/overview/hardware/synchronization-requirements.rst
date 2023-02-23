Synchronization Requirements
============================

The synchronization features provided by the ComputeMux runtime and compiler do
not impose any strict hardware requirements. ComputeMux's synchronization
features can all be implemented in software, but a ComputeMux implementation
**should** take advantage of any hardware support that exists for these
features. This section will briefly describe each of the synchronization
features present in ComputeMux, and what kind of hardware capabilities can be
beneficial to their implementation.

Execution Model
^^^^^^^^^^^^^^^

As outlined in the :ref:`execution model
overview<overview/runtime/computemux-runtime:Execution Model>`, ComputeMux's
work creation and execution APIs are designed to maximize device utilization by
allowing commands to be put into batches that can be executed on the device
without further intervention from a host CPU. In the simplest case this means
batching commands that don't have any dependencies between them, i.e. commands
that operate on different memory buffers. If there is a dependency between
commands, a ComputeMux implementation may still batch the commands together if
the device is capable of handling that dependency.

To illustrate this, consider the following scenario: a user is writing some
code to run on a device that consists of a host CPU, a DMA unit and an
accelerator. They define a memory transfer operation and a subsequent kernel
execution operation that works on the data transferred in via DMA. A ComputeMux
implementation will see these two operations and deduce the dependency between
them by checking which memory buffers they operate on. If the device has some
mechanism that allows the DMA unit to signal the accelerator to begin execution
when the memory transfer is complete, the ComputeMux implementation can take
advantage of this and have both commands execute together without synchronizing
with the host CPU in-between.

Any hardware feature that would enable this kind of scenario can be taken
advantage of when implementing the ComputeMux execution model to build more
complex dependency graphs from the commands issued at the open standard level.

Control Processors
^^^^^^^^^^^^^^^^^^

The features described in the previous :ref:`execution model
section<overview/hardware/synchronization-requirements:Execution Model>` do not
require a control processor. For systems that have one, however, ComputeMux
command execution can be mapped directly to it. A control processor is a small
processor within an acceleration device that is used to schedule work on the
compute units. Having such a processor can eliminate some host-device
synchronization requirements as the host CPU is generally able to send batches
of work to the acceleration device and the control processor manages the
dependencies.

Work is provided to the ComputeMux interface via command buffers, i.e. a
sequence of memory and execution commands. The commands in a single buffer are
to be executed as-if in-order, e.g. overlap is allowed if to independent data,
this can represent a sub-graph of the entire execution. A set of command
buffers have dependencies expressed between buffers, thus completing the
representation of the execution graph. The device specific internal
representation is usually also some form command buffer or command list. The
ComputeMux specification does not require any specific internal representation
for these buffers and graph, so a given implementation can follow the
representation native to its control processor and then offload an entire graph
of work to the device.

For programmers who wish to have control over how these command buffers work,
each language has an approach. In Vulkan all command execution is via a buffer
approach, in OpenCL there is a ``cl_khr_command_buffer`` extension, in SYCL
there are command group handlers and also multiple paths being examined in the
ecosytem for expressing larger graphs.

ComputeAorta's OpenCL implementation exploits OpenCL's execution model to
dynamically build up a graph of command buffers. The OpenCL runtime code that
ComputeAorta provides does not send a set of work to the ComputeMux
implementation until either some blocking operation occurs, or the programmer
explicitly flushes the OpenCL queue. This means that even OpenCL code that does
not explicitly use command buffers is still benefiting when there is a control
processor. This works well because the entire OpenCL interface is built around
the idea of asynchronously enqueuing work.

For systems without control processors this same structure does not have any
disadvantages, but still allows for re-ordering or combining commands if
beneficial. In the simplest case the commands can be executed one at a time,
but having visibility of an entire buffer of work allows for scheduling memory
transfers or enqueuing a sequence of kernels as a single command. This may
allow for higher utilization of hardware, or reduce the amount of
synchronization required.

Barrier Builtins
^^^^^^^^^^^^^^^^

The :ref:`barrier builtins<overview/compiler/ir:Barriers>` defined by
ComputeMux provide a limited ability to synchronize between kernel invocations
running on the device. While we fully support this in software via a compiler
pass, a ComputeMux target could make use of hardware features to implement this
functionality instead. The specifics of how this works will vary significantly
depending on how work execution and scheduling is implemented for a device. A
non-trivial implementation with hardware support will at least require some
ability to synchronize between two or more kernel execution threads such that
they can halt execution at a given point in a program and wait until all
threads have reached that point before continuing. A more detailed description
of different possible approaches to handling barriers, and how scheduling might
be implemented for a given device can be found in the
:doc:`/overview/example-scenarios/mapping-algorithms-to-vector-hardware`
section.

DMA Builtins
^^^^^^^^^^^^

ComputeMux defines a number of builtins that are designed to allow DMA
operations to be triggered from user compiled kernels. For a full description
of these see the relevant section of the :ref:`IR
overview<overview/compiler/ir:DMA>`. If an implementation chooses to implement
these functions by exposing access to some DMA capability of the underlying
hardware it **must** also support a mechanism to block execution of a kernel
until a specified DMA operation has completed. We represent this at the
ComputeMux level by having our DMA functions return ComputeMux event objects,
and exposing a function that allows a user to wait for one or more of these
event objects to be signalled by their associated DMA operations. This could be
implemented on the device by simply exposing some way to check the completion
status of a given DMA operation.
