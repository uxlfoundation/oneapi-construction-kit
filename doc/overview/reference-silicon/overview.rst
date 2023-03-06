Codeplay Reference Silicon Overview
====================================

Codeplay Reference Silicon (RefSi) or simply Codeplay Silicon is the name of a
computer platform developed by Codeplay to be used as a complete reference
target for running the Acoran compute stack on a RISC-V-based accelerator. It
can be seen as a System-On-Chip which includes components typically found in
hardware accelerator platforms, such as a CPU, DRAM, accelerator cores, fast
on-chip (TCM) memory, a DMA controller, a command processor and a bus to link
the components together.

Since RefSi is a virtual platform, many of its components are simulated.
Components that are not simulated are used as-is from a x86 machine (e.g. CPU,
system RAM) we call the 'host', in reference to the "offload execution model"
where the "Host CPU" offloads the key computations to an accelerator.

The platform is meant as an example for how Acoran maps to hardware with similar
components and features. It is used to demonstrate how such hardware
features can be integrated within Acoran components such as ComputeCpp,
ComputeAorta and libraries such as SYCL-BLAS and SYCL-DNN.

In this section we will describe the first version RefSi Accelerator
architecture (RefSi M1), its components and how these components interact. A
related but different reference architecture is RefSi G1, which is more CPU-like
and does not feature a DMA controller or command processor.

Architecture
------------

System CPU
^^^^^^^^^^

The RefSi M1 platform was designed with an 'offloading' execution model in mind,
which is exposed through standards like SYCL and OpenCL. In this model, the role
of the CPU is to run the 'host' part of compute applications. Typically, this is
logic that is not easily executed in parallel or that would not benefit from
highly parallel execution. The CPU is also responsible for scheduling and
dispatching work (in the form of compute kernels) to the accelerator. In some
scenarios, the CPU is also used to compile compute kernels down to machine code
that can be executed by accelerator cores.

Unlike many components in the RefSi platform, the CPU is not simulated. The x86
CPU that executes the Acoran for RISC-V software (the 'system CPU') is used
instead. There is no hard dependency on the x86 architecture however, and an ARM
or RISC-V CPU could equally well be used for this platform.

The details of this CPU are dependant on the host system where the RefSi
simulator runs.

System RAM
^^^^^^^^^^

This is the memory used by the host CPU. While not technically part of the RefSi
SoC, it is listed here to contrast with the DRAM that is part of the SoC. Due to
RefSi being a virtual platform, system RAM is used to simulate every kind of
memory that is exposed by the platform. However, accelerator cores cannot access
system RAM directly and must use DRAM instead. While the DMA controller could
technically be used by the cores to transfer data from system RAM, cores are not
expected to access any system pointers.

Size: dependent on the host system.

DRAM
^^^^

RefSi accelerators have their own dedicated RAM, separate from the system RAM
that is used by the CPU. DRAM can be accessed by the CPU, accelerator cores, the
DMA controller and the command processor.

This is the memory used to allocate global buffers when using SYCL or OpenCL.
DRAM can be pinned in the CPU's address space and used to transfer data between
the CPU and accelerator cores.

DRAM is also used to store command buffers that can be executed by the command
processor as well as kernel executables (e.g. ELFs) prior to being loaded in
an accelerator core's TCIM.

The DRAM size is set to 4 GB.

Accelerator Cores
^^^^^^^^^^^^^^^^^

Since their purpose is to execute compute kernels, accelerator cores are at the
heart of the RefSi platform which provides two 64-bit RISC-V cores, each with 4
hardware threads (called 'harts'). These 8 independent (i.e. no lockstep
execution) execution units provide some amount of parallelism while keeping the
system simple and easy to understand. Cores support the RVV extensible vector
extension to increase parallelism and demonstrate how Acoran can be mapped to
vector hardware.

Following typical accelerator architecture, the cores in the RefSi M1 platform
feature fast on-chip memory (tightly-coupled memory, or TCM) that can be used to
improve kernel performance by using tiling and double-buffering strategies. Each
core has a dedicated amount of TCM and DMA transfers to and from TCM are
supported.

While these RISC-V cores are capable of executing a general-purpose operating
system or even RTOS, for simplicity's sake they are only used to execute kernels
using a 'bare metal' approach. Following the 'offloading' style of execution
model exposed by OpenCL and SYCL, the cores are idle until the command processor
wakes them up with work and as soon as kernel execution is completed.

The ISA for RISC-V cores is set to `RV64GCVZbc`, the `VLEN` to 512 and `ELEN` to
64.

TCDM
^^^^

Each RISC-V accelerator core in the platform is associated with a chunk of fast
on-chip TCM (tightly-coupled memory) called. TCDM is meant for the storage of
data that is frequently accessed or requires low latency.

Size: 4 MB TCDM per core (data)

DMA Controller
^^^^^^^^^^^^^^

The DMA controller is responsible for managing data transfers between different
kinds of memory, such as system RAM, DRAM and accelerator cores' TCM. While
platform components such as the CPU and accelerator cores can access memory
directly, performing data transfers using the DMA controller reduces transfer
overheads since only a minimal number of instructions are needed to start a
transfer. Transfers are also asynchronous to other components (CPU, CMP, cores),
which increases parallelism within the platform.

The DMA controller includes a single FIFO queue of transfer requests, and two
DMA channels for performing DMA transfers. This means that up to two concurrent
data transfers are possible.

DMA transfers can be done between DRAM and TCM in both directions, between
system RAM and DRAM also in both directions, as well as within DRAM, so long as
the source and destination memory regions do not overlap. 1D, 2D and 3D
transfers are supported by the DMA controller.

The DMA controller can be accessed through the bus (e.g. by the CPU or
accelerator cores) or through the command processor using the same interface.

Accelerator cores can access the DMA controller through a set of DMA registers
that are mapped into the cores' address space. These registers allow data
transfers to be configured, started and waited for completion using regular load
and store instructions from the RISC-V ISA. Each hart has its own set of DMA
registers, allowing for transparent and independent access to the DMA controller
from OpenCL and SYCL kernels.

Command Processor
^^^^^^^^^^^^^^^^^

The command processor (CMP) can be seen as a scheduler for the RefSi M1
platform. Its purpose is to manage kernel execution as well as DMA transfers. It
is a simple programmable processor that can execute high-level commands such as
'Execute Kernel Slice' or 'Wait For Pending DMA Transfers To Complete'.

The CMP includes a FIFO queue of command requests, which are made up of a start
address and size for a command buffer. Command requests are executed in-order,
and commands specified within a command buffer are also executed in-order. Once
all commands in the command buffer have finished executing, the CMP signals the
CPU that a command request has been completed through the bus. The FIFO queue
size can contain a maximum of 16 requests. Attempting to submit a request when
the queue is full results the sender of the request to stall until an entry is
available in the queue.

In the first version of Codeplay Reference Silicon (M1) there is no plan to
include synchronization between different commands within a command buffer (e.g.
to implement compute graphs). However, this could be done in the future to allow
for overlapping kernel execution and DMA transfers.

Bus
^^^

The bus connects all components in the platform together, except for DRAM. The
CPU can use it to enqueue a command request onto the CMP FIFO as well as be
notified when the command processor has finished executing a request.

In the current revision of this design, the specific mechanism used by the bus
to communicate between components has not been explicitely specified.

Future Features
---------------

* Memory windows
* Dedicated matrix multiplication unit
* Scratchpad memory
* Command synchronisation
* Cross-core sync/barrier
