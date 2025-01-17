ComputeMux Runtime
==================

The ComputeMux Runtime defines an API providing an interface between hardware
target-specific code and general implementations of open standards such as
OpenCL and SPIR-V. Each entry point required for a hardware target in the
Runtime API is specified in the :doc:`ComputeMux Runtime Specification
</specifications/mux-runtime-spec>`, containing detail on the purpose of each
entry point, valid usage, and expected error codes.

Design
------

.. todo::
  Highlight any pertinent points that customer runtime engineers would care
  about regarding ComputeMux or comparison with other low-level APIs.

  Only brief description needed for end of October.

Execution Model
^^^^^^^^^^^^^^^

The goal of ComputeMux's execution model is to provide a basis for translating
work desribed at the open standard API level into driver code that maximizes
utilization of the device. This is achieved by establishing a simple set of
guarantees surrounding command execution at the ComputeMux level that allow our
open standard implementations (and, to an extent, the underlying ComputeMux
implementation) to batch work together so that many commands can execute on the
device without the need to synchronize with a host CPU. In ComputeMux when we
talk about a "command" we mean a discrete unit of work that might in some way
utilize the target device. This primarily means (but is not limited to) memory
operations, executions of user compiled kernels, and executions of builtin
kernels that make use of fixed function hardware.

ComputeMux commands are first pushed via API calls to command buffers, which
can be thought of as a list of :math:`N` commands that are guaranteed to
execute as if in the order they were pushed. Command buffers are then submitted
for execution on the device to command queues with a ``muxDispatch`` call.
Command buffers executing in a command queue do not have any implicit execution
order guarantees; instead, ComputeMux has a semaphore primitive to allow
synchronization between command buffers. When a command buffer is dispatched it
**may** be given a list of semaphores it **must** signal when it's finished,
and a list it **must** wait for before beginning execution.

From this brief summary of the ComputeMux command execution model we can
distill the two core guarantees that underpin the ComputeMux execution model:

* Commands within a command buffer **must** execute as if (in terms of
  side-effects) in the order they were pushed to the command buffer.
* Command buffers executing in a queue **must** wait for their designated
  semaphores to be signalled before beginning, and signal their designated
  semaphores when complete.

For a more detailed description of ComputeMux's execution model see the
:ref:`specifications/mux-runtime-spec:Execution Model` section of the
ComputeMux runtime spec. A ComputeMux implementation doesn't need any special
hardware features to support these guarantees, but batching of commands can be
improved by taking advantage of device features as described in the
:doc:`synchronization requirements
</overview/hardware/synchronization-requirements>` section.
