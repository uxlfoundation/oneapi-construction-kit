DMA Controller Reference
========================

This document describes the design of the interface exposed by the DMA
controller to RefSi execution units such as RISC-V accelerator cores and the
CMP.

Register Reference
------------------

Performing DMA operations involves reading from and writing to a number of
memory-mapped registers. These registers are implemented in the RISC-V
accelerator cores on a per-hart basis, i.e. each hart has its own DMA context
that can be used independently of other harts. This means in practice two harts
reading a register from the same memory address may see different values at the
same point in time. These registers can also be accessed through the CMP with
commands such as ``WRITE_REG64``, though it should be noted that the CMP has its
own set of DMA registers that are independent of the RISC-V harts.

Similarly to other memory-mapped registers, partial writes or reads are not
allowed. The entire register must be read from or written to in a single memory
operation.

Some of the registers described here are register arrays, which can be identified
by the 'n' at the end of their names (e.g. ``DMAXFERSIZEn``). Register arrays
contain several registers with the same layout and semantics, but holding
different values. For example ``DMAXFERSIZEn`` is an array of three registers,
holding transfer sizes in the first, second and third dimensions, respectively.

``DMA Source Address`` register (``DMASRCADDR``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This register stores the source address for the DMA operation. The memory at the
address can reside in either global (DDR) or local (cluster shared memory)
memory. The width and layout of this register is the same as for a pointer
stored in a general-purpose register.

Fields:

* Bits 63-0: Source address for the DMA operation.

``DMA Destination Address`` register (``DMADSTADDR``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This register stores the destination address for the DMA operation. The memory
at the address can reside in either global or local memory, but there is no
requirement that the source and destination be in different kinds of memory.
There must be no overlap between the source and destination regions, however.

Fields:

* Bits 63-0: Destination address for the DMA operation.

``DMA Transfer Size`` register (``DMAXFERSIZEn``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This register array stores the number of bytes to copy from the source to the
destination. It contains 3 registers (``n`` between 0 and 2), one for each
dimension of the transfer. This enables 1D, 2D and 3D transfers to be performed.

In practice, 2D and 3D transfers are only used for non-sequential (i.e. strided)
operations. This is because a sequential 2D or 3D transfer can be expressed in
terms of a sequential 1D transfer.

Fields:

* Bits 63-0: Size of the copy in the ``n``-th dimension. For dimension zero it
  is a byte count, whereas for higher dimensions it is an element count (e.g.
  row count, slice count).

``DMA Transfer Source Stride`` register (``DMAXFERSRCSTRIDEn``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This register array stores the source strides to use for the transfer and
contains 2 registers (``n`` between 0 and 1). This enables 2D and 3D strided
transfers to be performed.

Strides are only used for certain types of transfers, when the stride mode is
either 'source' or 'multi-stride'.

Fields:

* Bits 63-0: Stride of the transfer in source memory, in bytes, for the
  ``n``-th dimension.

``DMA Transfer Destination Stride`` register (``DMAXFERDSTSTRIDEn``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This register array stores the destination strides to use for the transfer and
contains 2 registers (``n`` between 0 and 2). This enables 2D and 3D strided
transfers to be performed.

Strides are only used for certain types of transfers, when the stride mode is
either 'destination' or 'multi-stride'.

Fields:

* Bits 63-0: Stride of the transfer in destination memory, in bytes, for the
  ``n``-th dimension.

``DMA Transfer Control`` register (``DMACTRL``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This registers stores DMA transfer configuration settings. Writing a value to
this register where the LSB is set triggers a new DMA operation. DMA registers
can be written to while DMA transfers are active, however this has no effect on
pending transfers.

Fields:

* Bit 0: Used to start a new DMA operation by writing `1`. Always reads `0`.
* Bits 5-4: Dimension of the transfer. `0b01`: 1D, `0b10`: 2D, `0b11`: 3D, `00`:
  reserved.
* Bits 7-6: Stride mode. `0b00`: no stride, `0b10`: source stride, `0b01`: destination
  stride, `0b11`: multi-stride.

``DMA Started Sequence`` register (``DMASTARTSEQ``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This register holds the identifier of the DMA transfer that was most recently
started by the current hart. DMA transfer IDs are generated sequentially upon
enqueuing a new DMA transfer and are used to wait for one or more transfers to
be complete.

On overflow (i.e. when the identifier of the most recently started transfer is
the largest representable number for the 32-bit field), the next generated
identifier will be greater than zero.

This is a read-only register. Writes to this register have no effect.

Fields:

* Bits 31-0: Identifier of the most recently started DMA transfer for the hart.

``DMA Completed Sequence`` register (``DMADONESEQ``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This register holds the current value of the 'DMA Completed Transfer' sequence,
which is incremented every time a DMA transfer is completed and all previously
enqueued transfers (i.e. with lower transfer IDs) have also been completed. DMA
transfer IDs are generated sequentially upon enqueuing a new DMA transfer and
are used to wait for one or more transfers to be complete.

Reading this register queries the latest value of the 'DMA Completed Transfer'
sequence. Writing a DMA transfer ID to this register blocks the hart until the
specified DMA transfer (as well as all transfers with lower IDs) is complete. If
the ID written to the register is greater than the value in the
`DMA Started Sequence` register, the hart is blocked until all upstanding
transfers have been completed. This is to handle transfer ID overflows.

DMA transfers do not have to complete in the order that they have been enqueued.
However, reading this register will only result in a value of ``N`` if all
transfers with an identifier less than or equal to ``N`` are complete. Writing a
value of ``N`` to this register works in the same way and will block until all
transfers (up to and including ``N``) are complete.

When the ``DMASTARTSEQ`` register overflows, new transfer IDs will be less than
older transfer IDs. In this case, writing a 'lower' ID may complete before older
transfers with 'higher' IDs are finished. This has to be taken into account
when waiting for multiple transfers, for example by writing the largest transfer
ID as well as the lowest ID when an overflow is detected. Waits of individual
transfers are not affected.

Fields:

* Bits 31-0: Current value of the `DMA Completed Transfer` sequence.

