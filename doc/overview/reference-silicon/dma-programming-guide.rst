DMA Programming Guide
======================

The goal of the Kernel DMA feature is to enable the user to start DMA operations
from kernels when executing them on RefSi M1 and wait for these operations
to complete. DMA operations started this way transfer data between global and
local memory. It is intended to be a reference implementation that can be used
to understand how such a feature can be implemented and exposed through
the oneAPI Construction Kit.

One use case for Kernel DMA is to implement tiling of data stored in global
buffers, using local memory as temporary storage for the data tiles. When DMA is
supported by the hardware, it is typically a much faster way of transferring
data compared to manually fetching the data using regular load and store
instructions in the kernel, which potentially goes through a data cache. Data
transfers should happen asynchronously, so that double-buffering techniques can
be used to perform data transfer in parallel with computation in the kernel.

Operations
----------

Two new operations are introduced with this feature: start DMA operation and
wait for DMA operation to finish. Both operations can be triggered in the same
way as existing IO-mapped operations such as 'exit', 'print' and 'barrier'. This
is done by either writing to or reading from registers in the `dma_io` memory
region (`0x20002000` to `0x200020ff` for RefSi M1).

Start DMA Operation
^^^^^^^^^^^^^^^^^^^

Starting a new DMA operation can be done by setting the LSB of the `DMA Transfer
Control` register to one. When this happens, all of the DMA-related registers
for that hart are read and a DMA operation is enqueued. Once a transfer has been
started on a hart, further transfers can be started without having to wait for
the first transfer to finish. This allows for multiple DMA transfers to be
outstanding for a hart at any given time. The maximum number of concurrent
transfers is implementation-dependent.

Each write to this register with the LSB set results in a copy of the DMA
registers, so that existing transfers are not affected. However, writes may
block if the number of maximum concurrent transfers is reached and until the new
transfer can be added to the queue.

After starting a transfer, it is common to immediately read the `DMA Started
Sequence` register to retrieve the ID allocated for the transfer.

Wait For DMA Operation to Finish
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Waiting for a previously-started DMA operation to complete can be done by
repeatedly reading the `DMA Completed Sequence` register until the returned
value is greater or equal to the DMA transfer ID. An alternative approach is to
write the DMA transfer ID to the `DMA Completed Sequence` register, which blocks
the hart until the corresponding DMA transfer is complete.

Example Usage
-------------

Since the registers used to manage DMA transfers are memory-mapped, there is no
need for compiler intrinsics in order to start and wait for DMA transfers. Here
is a C example of how to start a 1D transfer::

    #include "device/dma_regs.h"

    volatile uintptr_t *dma_regs = (volatile uintptr_t *)REFSI_DMA_IO_ADDRESS;

    // Configure and start a sequential 1D DMA transfer.
    uint64_t config = REFSI_DMA_1D | REFSI_DMA_STRIDE_NONE;
    dma_regs[REFSI_REG_DMASRCADDR] = (uintptr_t)src;
    dma_regs[REFSI_REG_DMADSTADDR] = (uintptr_t)dst;
    dma_regs[REFSI_REG_DMAXFERSIZE0] = size_in_bytes;
    dma_regs[REFSI_REG_DMACTRL] = config | REFSI_DMA_START;

    // Retrieve the transfer ID.
    uint32_t xfer_id = dma_regs[REFSI_REG_DMASTARTSEQ];

The simulator trace below, mixed with RefSi HAL debugging output, should
give an idea of how starting a DMA operation looks like from a RISC-V hart's
perspective::

    core   0: 0x0000000000001916 (0x200027b7) lui     a5, 0x20002
    core   0: 0x000000000000191a (0x0000ef8c) c.sd    a1, 24(a5)
    dma_device_t::write_dma_reg() Set source address to 0xffffee00
    core   0: 0x000000000000191c (0x0000f388) c.sd    a0, 32(a5)
    dma_device_t::write_dma_reg() Set destination address to 0xffffde00
    core   0: 0x000000000000191e (0x0000f790) c.sd    a2, 40(a5)
    dma_device_t::write_dma_reg() Set transfer size to 0x200 bytes
    core   0: 0x0000000000001920 (0x05100713) li      a4, 81
    core   0: 0x0000000000001924 (0x0000e398) c.sd    a4, 0(a5)
    dma_device_t::do_kernel_dma() Started 1D transfer with ID 16
    core   0: 0x0000000000001926 (0x00006788) c.ld    a0, 8(a5)
    dma_device_t::read_dma_reg() Most recent transfer ID: 16
    core   0: 0x0000000000001928 (0x00008082) ret

