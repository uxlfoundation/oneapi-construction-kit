RefSi In-Kernel DMA for RISC-V
==============================

When the RefSi HAL is used to execute OpenCL and SYCL kernels on the Spike
simulator, the RISC-V target supports a mechanism that allows kernels to perform
data transfers between different kinds of simulated memory. This feature is
called RefSi In-Kernel DMA and it serves as an example of how
accelerator-controlled DMA can be integrated in ComputeAorta and ComputeCpp for
existing hardware. DMA transfers can be configured, started and waited for
through an interface consisting of memory-mapped registers. The current
implementation simulates memory transfers at a high level, not taking into
account memory bandwidth nor being tied to the RISC-V pipeline.

Purpose
-------

Accelerators use different kinds of memory with different characteristics, such
as latency, size and location. For example, it is common for accelerator cores
to include on-chip tightly-coupled memory (TCM) that has much lower latency than
DRAM, which is typically in the global memory address space in OpenCL and SYCL.
As such, ensuring that the majority of kernel memory accesses are done through
TCM and not DRAM is often a priority when optimizing kernels.

However, TCM is also typically orders of magnitude smaller than DRAM and it is
rarely large enough to fit the entire data being processed by the kernel. In
order to work around this limitation, a 'streaming' approach can be used where
computation is done on a smaller amount of data (tiles) that does fit in TCM.
With In-Kernel DMA, kernels can perform asynchronous data transfers between DRAM
and TCM. As a result, fast memory can be used while also being able to address
any amount of data from DRAM (i.e. more than one tile per kernel invocation) and
reducing kernel overhead by processing multiple tiles per invocation. Since
transfers are asynchronous, copying data can also be done concurrently with
computation (e.g. double buffering) to further improve performance.

How It Works
------------

In-Kernel DMA is exposed to RISC-V cores as a set of memory-mapped DMA registers
that can be used to configure, start and wait for DMA transfers to complete. The
RefSi HAL monitors accesses to the area of memory that contains DMA registers
while kernels are being executed on the Spike simulator and performs transfers
when they are requested by a RISC-V core. This allows standard RISC-V load and
store instructions to be used for managing DMA transfers instead of having to
use proprietary instructions.

The following set of registers are available:

* `DMA Source Address` register (`DMASRCADDR`)
* `DMA Destination Address` register (`DMADSTADDR`)
* `DMA Transfer Size` register (`DMAXFERSIZEn`)
* `DMA Transfer Stride` register (`DMAXFERSTRIDEn`)
* `DMA Transfer Control` register (`DMACTRL`)
* `DMA Started Sequence` register (`DMASTARTSEQ`)
* `DMA Completed Sequence` register (`DMADONESEQ`)

The first four registers are used to configure specific parameters of a DMA
transfer (addresses, size and stride) while `DMACTRL` allows new DMA transfers
to be started using the previously-configured parameters. The `DMASTARTSEQ`
register allows the ID of the most recently started DMA transfer to be
retrieved, while `DMADONESEQ` can be used to wait for one or more transfers to
complete.

Starting a new transfer can be done by writing DMA parameters to the
corresponding registers, writing to the DMA Transfer Control register to set the
`START` bit and then reading from the DMA Started Sequence register in order to
retrieve the ID of the new transfer.

Waiting for a transfer to complete can be done by writing the ID of the transfer
to the DMA Completed Sequence register, which stops execution of the core until
the transfer is complete. A non-blocking polling approach can also be used by
reading the DMA Completed Sequence register repeatedly until the loaded value is
greater than or equal to the transfer ID. Multiple DMA transfers can be active
at the same time and 'waited for' in a single operation.


How To Use
----------

While In-Kernel DMA is exposed to the simulated RISC-V machine as a set of
memory-mapped registers, this interface is hidden from end-users. DMA transfers
can be started and waited for in kernels by using built-in functions exposed by
the compute framework being used.

OpenCL
******

In OpenCL kernels, In-Kernel DMA can be accessed through the standard
`async_work_group_copy`, `async_work_group_strided_copy`, and
`wait_group_events` built-in functions.::

    #define N 128

    kernel void tiled_computation(global uint *src, global uint *dst) {
      size_t idx = get_group_id(0);
      __local uint tile[N];

      event_t event = async_work_group_copy(&tile[0], &src[N * idx], N, 0);
      wait_group_events(1, &event);

      // ... do something with the tile ...

      event_t event2 = async_work_group_copy(&dst[N * idx], &tile[0], N, 0);                    ·
      wait_group_events(1, &event);
    }

SYCL
****

In SYCL kernels, In-Kernel DMA can be accessed through the 
`async_work_group_copy` and `wait_for` methods of the `sycl::nd_item` class.::

    void operator()(sycl::nd_item<1> item) {
      size_t offset = item.get_group_id(0) * N;
      auto ev1 = item.async_work_group_copy(tile.get_pointer(),​
                                            accSrc.get_pointer() + offset,​ N);​
      item.wait_for(ev1);

      // ... do something with the tile ...

      auto ev2 = item.async_work_group_copy(accDst.get_pointer() + offset,​
                                            tile.get_pointer(),​ N);​
      item.wait_for(ev2);
    }

Implementation
--------------

This section describes how the In-Kernel DMA feature was implemented for the
RISC-V ComputeMux target and RefSi HAL, as an example of how In-Kernel DMA can
be implemented for other targets using hardware DMA capabilities. The
implementation can be divided into two parts: compiler and simulator.

Compiler
********

As we have seen in the previous section, In-Kernel DMA is exposed to the user as
a set of OpenCL and SYCL built-in functions which have no implementation (i.e.
their body is empty). In order for DMA to function properly these functions need
to be replaced with functions that control the target's DMA functionality.

This is done by adding a compiler pass, `RefSiReplaceMuxDmaPass`, to the
RISC-V's pass pipeline. When this pass is run, it replaces the ComputeMux
equivalent of OpenCL and SYCL DMA built-in functions with RefSi DMA built-in
functions:

* `__mux_dma_(read|write)_1D` is replaced with `_refsi_dma_start_seq_(read|write)`
* `__mux_dma_(read|write)_2D` is replaced with `_refsi_dma_start_2d_(read|write)`
* `__mux_dma_(read|write)_3D` is replaced with `_refsi_dma_start_3d_(read|write)`
* `__mux_dma_wait` is replaced with `_refsi_dma_wait`

RefSi DMA built-in functions are generated using the LLVM IR API. They mainly
contain load and store instructions that access the memory-mapped registers::

    // Write a value to the DMA register specified by the register index.
    static llvm::Instruction *writeDmaReg(llvm::IRBuilder<> &builder,
                                          unsigned regIdx, llvm::Value *val) {
      auto *regAddr = getDmaRegAddress(builder, regIdx);
      val = getDmaRegVal(builder, val);
      return builder.CreateStore(val, regAddr, /* isVolatile */ true);
    }

    // Read a value from the DMA register specified by the register index.
    static llvm::Value *readDmaReg(llvm::IRBuilder<> &builder, unsigned regIdx) {
      auto *regAddr = getDmaRegAddress(builder, regIdx);
      llvm::Type *regTy = getDmaRegTy(builder.getContext());
      return builder.CreateLoad(regTy, regAddr, /* isVolatile */ true);
    }

    ...

      IRBuilder<> &builder;

      // Set the destination address.
      writeDmaReg(builder, REFSI_REG_DMADSTADDR, dstAddr);

      // Set the source address.
      writeDmaReg(builder, REFSI_REG_DMASRCADDR, srcAddr);

      // Set the transfer size.
      writeDmaReg(builder, REFSI_REG_DMAXFERSIZE0, size);  // Bytes

      // Configure and start a sequential 1D DMA transfer.
      uint64_t config = REFSI_DMA_1D | REFSI_DMA_SEQUENTIAL | REFSI_DMA_START;
      writeDmaReg(builder, REFSI_REG_DMACTRL,
                  llvm::ConstantInt::get(dmaRegTy, config));

      // Retrieve the transfer ID and convert it to an event.
      auto *xferId = readDmaReg(builder, REFSI_REG_DMASTARTSEQ);
      auto *event = builder.CreateIntToPtr(xferId, func->getReturnType());
      builder.CreateRet(event);

    ...

This results in IR like the following for a 1D 'write' DMA transfer that copies
data from global memory to local memory::

    define %__mux_dma_event_t*
    @__spike_dma_start_seq_write(i8 addrspace(1)* %dst, i8 addrspace(3)* %src,
                                 i64 %width.bytes, %__mux_dma_event_t* %event) {
    entry:
      %8 = ptrtoint i8 addrspace(1)* %dst to i64
      store volatile i64 %8, i64* inttoptr (i64 1073750048 to i64*)
      %9 = ptrtoint i8 addrspace(3)* %src to i64
      store volatile i64 %9, i64* inttoptr (i64 1073750040 to i64*)
      store volatile i64 %width.bytes, i64* inttoptr (i64 1073750056 to i64*)
      store volatile i64 81, i64* inttoptr (i64 1073750016 to i64*)
      %10 = load volatile i64, i64* inttoptr (i64 1073750024 to i64*)
      %11 = inttoptr i64 %10 to %__mux_dma_event_t*
      ret %__mux_dma_event_t* %11
    }

As can be seen in the IR code above, RefSi In-Kernel DMA can be used with purely
standard RISC-V instructions due to its memory-mapped interface, where registers
are accessed by reading from and writing to memory at specific predetermined
locations. Other hardware DMA implementations may require using DMA-specific 
instructions at the assembly level, in which case they would likely be generated
by the DMA replacement pass as a compiler intrinsic.

Simulator
*********

The second part of the RefSi In-Kernel DMA feature is implemented in the Spike
simulator that is included by the RefSi HAL, so that accessing the DMA registers
like was described in the previous sub-section actually results in a data
transfer. This is only done when the target is simulated, as real hardware will
have the DMA functionality implemented in silicon.

This is done by registering the memory range used by DMA registers with the
Spike simulator as a 'special device'. When the simulated RISC-V core accesses a
DMA register using a regular load or store instruction, the instruction's
default behaviour is disabled (otherwise a memory trap would occur) and one of
the following two 'hook' functions is called by the simulator::

    bool dma_device_t::load(reg_t addr, size_t len, uint8_t *bytes) override;​

    bool dma_device_t::store(reg_t addr, size_t len, const uint8_t *bytes) override;​

These 'hook' functions are responsible for performing the required behaviour of
the hooked instructions in the simulator, such as reading from and writing to
the simulated DMA registers and performing the DMA transfers. In our case the
`dma_device_t` class that was registered with the 'DMA register' memory range
holds a set of DMA registers for all RISC-V harts. When writing a value that has
the `REFSI_DMA_START` bit set to the `REFSI_REG_DMACTRL` register, the `store`
hook will perform a DMA transfer using the parameters previously set by writing
to the respective DMA parameter registers::

    bool dma_device_t::do_kernel_dma_1d(size_t hart_id, uint8_t *dst_mem,
                                        uint8_t *src_mem) {
      uint64_t *dma_regs = get_dma_regs(hart_id);

      // Retrieve the size of the transfer.
      reg_t size = dma_regs[REFSI_REG_DMAXFERSIZE0];

      ...

      // Allocate a new ID for the transfer.
      uint32_t xfer_id = (uint32_t)dma_regs[REFSI_REG_DMASTARTSEQ] + 1;
      dma_regs[REFSI_REG_DMASTARTSEQ] = xfer_id;

      // Perform the transfer.
      memcpy(dst_mem, src_mem, size);

      // Mark the transfer as completed.
      dma_regs[REFSI_REG_DMADONESEQ] = xfer_id;

      return true;
    }

In the code above, `dst_mem` and `src_mem` point to memory that is mapped in the
RISC-V cores' address space, so that performing a `memcpy` using these pointers
results in a data transfer in the simulated RISC-V device's memory.


Troubleshooting
***************

Since kernels that make use of DMA often include complicated addressing code and
asynchronous DMA introduces the opportunity for timing issues, debugging tools
can be very useful to troubleshoot kernels that do not produce the expected
outputs.

DMA Tracing
***********

DMA tracing can be enabled by setting the `CA_HAL_DEBUG` environment variable to
`1` prior to executing the program to troubleshoot. The program will then print
debugging information to the console when DMA registers are accessed and DMA
transfers are started::

    $ CA_HAL_DEBUG=1 bin/dma_concatenate
    ...
    dma_device_t::write_dma_reg() Set destination address to 0x408ffd80
    dma_device_t::write_dma_reg() Set source address to 0xfffffc00
    dma_device_t::write_dma_reg() Set transfer size to 0x40 bytes
    dma_device_t::do_kernel_dma() Started 1D transfer with ID 1
    dma_device_t::read_dma_reg() Most recent transfer ID: 1
    dma_device_t::write_dma_reg() Set destination address to 0x408ffd00
    dma_device_t::write_dma_reg() Set source address to 0xfffff800
    dma_device_t::write_dma_reg() Set transfer size to 0x40 bytes
    dma_device_t::do_kernel_dma() Started 1D transfer with ID 2
    dma_device_t::read_dma_reg() Most recent transfer ID: 2
    dma_device_t::write_dma_reg() Waiting for transfer ID 2
    dma_device_t::read_dma_reg() Most recent transfer ID: 2
    dma_device_t::read_dma_reg() Most recent transfer ID: 2
    dma_device_t::write_dma_reg() Waiting for transfer ID 2
    ...

Assembly Output
***************

Another useful troubleshooting feature is the ability to inspect the RISC-V
assembly code generated by the compiler prior to executing a kernel. This can be
done by setting the `CA_RISCV_DUMP_ASM` environment variable to `1` before
running the program::

    $ CA_RISCV_DUMP_ASM=1 bin/dma_concatenate
    ...

    __refsi_dma_start_2d_gather:
            or      a3, a3, a4
            or      a3, a3, a5
            beqz    a3, .LBB0_2
            lui     a0, 262146
            addiw   a0, a0, 8
            ld      a0, 0(a0)
            ret
    .LBB0_2:
            lui     a3, 262146
            addiw   a4, a3, 32
            sd      a0, 0(a4)
            addiw   a0, a3, 24
            sd      a1, 0(a0)
            addiw   a0, a3, 40
            addi    a1, zero, 4
            sd      a1, 0(a0)
            addiw   a0, a3, 48
            sd      a2, 0(a0)
            addiw   a0, a3, 64
            sd      a1, 0(a0)
            addi    a0, zero, 161
            sd      a0, 0(a3)
            lui     a0, 262146
            addiw   a0, a0, 8
            ld      a0, 0(a0)
            ret

    __refsi_dma_wait:
            slli    a0, a0, 32
            srli    a0, a0, 32
            lui     a1, 262146
            addiw   a1, a1, 16
            sd      a0, 0(a1)
            ret

    ...
