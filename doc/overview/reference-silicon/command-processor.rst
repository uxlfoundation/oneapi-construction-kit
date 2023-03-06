Command Processor Reference
===========================

Commands such as N-D range kernel execution or DMA transfers are not directly
accessible through driver API calls such as the ones described in the 'Driver
Functions' section. Instead, they can be encoded into an array called a command
buffer and passed to the `refsiExecuteCommandBuffer` driver function for
execution. The commands will be passed to the RefSi command processor (CMP) and
executed in-order.

General Command Format
----------------------

Commands in a command buffer are serialized using binary format based on the
`PM4 packet format`_ used on several Radeon and `Adreno`_ GPUs:

..  _pm4 packet format: https://developer.amd.com/wordpress/media/2013/10/si_programming_guide_v2.pdf
..  _adreno: https://bakhi.github.io/mobileGPU/adreno/

These packets are made up of one or more 64-bit chunks of data. The first chunk
is always a header and it is followed by zero or more 64-bit payload chunks.
Little-endian ordering is used to encode the chunks.

The header contains the following fields:

* Bits 7-0: Reserved. Set to zero.
* Bits 15-8: Opcode. Determine the type of RefSi command to execute.
* Bits 29-16: Count. Equal to the number of payload chunks, times two.
* Bits 31-30: Packet identifier. Always set to 3.
* Bits 63-32: Inline chunk. Command-dependent meaning.

The format of payload chunks differs based on the actual command to be executed
and will be described in the following section. Unless otherwise specified,
fields are unsigned integers. Zero-extension must be used when storing an
unsigned field in a chunk that is larger than the field.

Opcodes are described in tables in which ``Chunk`` can be the header's
``Inline`` chunk or denotes one or multiple 64-bit data payload chunks.
``Field`` is the opcode-specific name of the encoded data.

Commands
--------

`FINISH` command (Opcode: 1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command marks the end of the command buffer. A 'command buffer complete'
signal will be sent on the bus and the next command buffer in the queue will be
executed.

+--------+-------------------+-------------------------------------------------+
| Chunk  | Field             | Description                                     |
+========+===================+=================================================+
| Inline | DUMMY             | This field is not used.                         |
+--------+-------------------+-------------------------------------------------+

`WRITE_REG64` command (Opcode: 2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command writes a 64-bit value to a CMP register.

+--------+------------------+--------------------------------------------------+
| Chunk  | Field            | Description                                      |
+========+==================+==================================================+
| Inline | REG_IDX          | CMP register index to write the 64-bit value to. |
+--------+------------------+--------------------------------------------------+
| 1      | IMM_VAL          | 64-bit immediate value to write to the register. |
+--------+------------------+--------------------------------------------------+

`LOAD_REG64` command (Opcode: 3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command reads a 64-bit value from the given memory address and writes it to
a CMP register. The memory address must be accessible by the CMP.

+--------+------------------+--------------------------------------------------+
| Chunk  | Field            | Description                                      |
+========+==================+==================================================+
| Inline | REG_IDX          | CMP register index to write the 64-bit value to. |
+--------+------------------+--------------------------------------------------+
| 1      | SRC_ADDR         | Memory address to read the 64-bit value from.    |
+--------+------------------+--------------------------------------------------+

`STORE_REG64` command (Opcode: 4)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command reads a 64-bit value from a CMP register and writes it to the given
memory address. The memory address must be accessible by the CMP.

+--------+------------------+--------------------------------------------------+
| Chunk  | Field            | Description                                      |
+========+==================+==================================================+
| Inline | REG_IDX          | CMP register index to read the 64-bit value from.|
+--------+------------------+--------------------------------------------------+
| 1      | DST_ADDR         | Memory address to write the 64-bit value to.     |
+--------+------------------+--------------------------------------------------+

`STORE_IMM64` command (Opcode: 5)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command writes an immediate 64-bit value to the given memory address. The
memory address must be accessible by the CMP and only 32-bit destination
addresses are supported with this command. It can be used for example to access
the DMA controller via the same register interface that is exposed to the RISC-V
accelerator cores.

+--------+------------------+--------------------------------------------------+
| Chunk  | Field            | Description                                      |
+========+==================+==================================================+
| Inline | DST_ADDR_LO      | Lower 32 bits of the address to write to.        |
+--------+------------------+--------------------------------------------------+
| 1      | IMM_VAL          | 64-bit immediate value to write to memory.       |
+--------+------------------+--------------------------------------------------+

`COPY_MEM64` command (Opcode: 6)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command copies a number of 64-bit elements from one area of memory to
another. Elements are copied one by one, to enable this function to be used with
memory-mapped registers such as performance counters. The `UNIT` field can be
used to read data from a particular hart in the RefSi accelerator.

+--------+------------------+--------------------------------------------------+
| Chunk  | Field            | Description                                      |
+========+==================+==================================================+
| Inline | COUNT            | Number of 64-bit elements to copy.               |
+--------+------------------+--------------------------------------------------+
| 1      | SRC_ADDR         | 64-bit address to read data from.                |
+--------+------------------+--------------------------------------------------+
| 2      | DST_ADDR         | 64-bit address to write data to.                 |
+--------+------------------+--------------------------------------------------+
| 3      | UNIT             | Unit ID to use for reading data from memory.     |
+--------+------------------+--------------------------------------------------+

`RUN_KERNEL_SLICE` command (Opcode: 7)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command runs a kernel slice, executing the entry point function
`NUM_INSTANCES` times with the same kernel arguments and slice ID. An instance
ID ranging from zero to `NUM_INSTANCES - 1` is passed to the entry point
function in order to identify the instance being currently executed.

Before the entry point function is executed on the accelerator, this command can
optionally copy a memory range located in device memory (e.g. TCIM) to the
per-thread Kernel Thread Block located in TCDM. This can be used for example to
populate the per-thread scheduling info structure based on a single scheduling
info template located in device memory.

The entry point function is executed across RISC-V accelerator cores and
hardware threads, with the CMP being responsible for distributing the instances
between available threads. Kernel execution happens synchronously, so that
following commands have to wait until this command has finished executing before
they are executed.

The following  registers are implicitly read by this command and therefore must
be set to valid values before the command is executed.

* `CMP_ENTRY_PT_FN`
* `CMP_KUB_DESC`
* `CMP_KARGS_INFO`
* `CMP_TSD_INFO`
* `CMP_STACK_TOP`
* `CMP_RETURN_ADDR`

Changes to device memory made by the cores when running the kernel will be fully
observable before the next command is executed, i.e. caches are implicitly
flushed.

+--------+-------------------+------------------------------------------------------------+
| Chunk  | Field             | Description                                                |
+========+===================+============================================================+
| Inline | [7:0] MAX_HARTS   | Maximum number of harts to run the kernel on.              |
+--------+-------------------+------------------------------------------------------------+
| 1      | NUM_INSTANCES     | Number of times to call the kernel entry point function.   |
+--------+-------------------+------------------------------------------------------------+
| 2      | SLICE_ID          | Identifier to pass to the kernel entry point.              |
+--------+-------------------+------------------------------------------------------------+

`RUN_INSTANCES` command (Opcode: 8)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command executes a kernel function `NUM_INSTANCES` times with a variable
number of arguments. An instance ID ranging from zero to `NUM_INSTANCES - 1` is
passed to the entry point function as the first argument in order to identify
the instance being currently executed. Up to 7 extra arguments can be passed to
the entry point function in the order they are defined in the `EXTRA_ARG_x`
chunks.

It is similar to `RUN_KERNEL_SLICE` but does not mandate a fixed kernel entry
point signature, other than the instance ID being the first parameter. Unlike
with `RUN_KERNEL_SLICE`, Kernel Uniform Block and Kernel Thread Block data is
not copied to TCDM. This must be done explicitly prior to executing this
command.

The entry point function is executed across RISC-V accelerator cores and
hardware threads, with the CMP being responsible for distributing the instances
between available threads. Kernel execution happens synchronously, so that
following commands have to wait until this command has finished executing before
they are executed.

The following  registers are implicitly read by this command and therefore must
be set to valid values before the command is executed.

* `CMP_ENTRY_PT_FN`
* `CMP_STACK_TOP`
* `CMP_RETURN_ADDR`

Changes to device memory made by the cores when running the kernel will not
always be fully observable before the next command is executed, i.e. caches need
to be explicitly flushed using the `SYNC_CACHE` command.

+--------+-------------------+------------------------------------------------------------+
| Chunk  | Field             | Description                                                |
+========+===================+============================================================+
| Inline | [7:0] MAX_HARTS   | Maximum number of harts to run the kernel on.              |
| Inline | [10:8] NUM_ARGS   | Number of extra function arguments (max 7).                |
+--------+-------------------+------------------------------------------------------------+
| 1      | NUM_INSTANCES     | Number of times to call the kernel entry point function.   |
+--------+-------------------+------------------------------------------------------------+
| 2      | EXTRA_ARG_0       | Value to pass to the entry point function as argument.     |
+--------+-------------------+------------------------------------------------------------+
| ...    | ...               | ...                                                        |
+--------+-------------------+------------------------------------------------------------+
| n      | EXTRA_ARG_n       | Value to pass to the entry point function as argument.     |
+--------+-------------------+------------------------------------------------------------+

`SYNC_CACHE` command (Opcode: 9)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command synchronizes one or more caches in the RefSi device with the host.
After this command has executed, all data written to RISC-V harts' caches is
flushed to device memory. Any data cached for reading is also invalidated.

+--------+------------------+--------------------------------------------------+
| Chunk  | Field            | Description                                      |
+========+==================+==================================================+
| Inline | FLAGS            | Flags describing which caches to synchronize.    |
+--------+------------------+--------------------------------------------------+

Possible flags:

* ``CMP_CACHE_SYNC_ACC_DCACHE``: Synchronize the data cache.
* ``CMP_CACHE_SYNC_ACC_ICACHE``: Synchronize the instruction cache.

Registers
---------

CMP_SCRATCH Register (ID: 0)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `CMP_SCRATCH` register has no special purpose and can be used freely in a
command buffer. For example, it could be used to temporarily store the most
recent transfer ID after starting a new DMA transfer. By writing this stored
transfer ID to the appropriate DMA register using the `STORE_REG64` command, the
CMP will wait until this transfer is complete before executing the next command.

+--------+-------------------+-------------------------------------------------+
| Bits   | Field             | Description                                     |
+========+===================+=================================================+
| 63-0   | SCRATCH           | No meaning.                                     |
+--------+-------------------+-------------------------------------------------+

CMP_ENTRY_PT_FN (ID: 1)
^^^^^^^^^^^^^^^^^^^^^^^

The `CMP_ENTRY_PT_FN` register holds the address of the kernel entry point in
device memory and must be set before executing any kernel command.

When using the `RUN_INSTANCES` command (the preferred way to execute kernels),
the entry point must have the following signature::

    void kernel_entry(size_t instance_id, void *arg1, ..., void *arg7);

The `instance_id` parameter is set by the CMP to identify the kernel instance
being executed by the hardware thread.

When using the `RUN_KERNEL_SLICE` command the entry point must have the
following signature instead::

    void kernel_entry(size_t instance_id, size_t slice_id,
                      const void *packed_args, void *ktb);

The `instance_id` parameter is set by the CMP to identify the kernel instance
being executed by the hardware thread, while the `slice_id` parameter is set
within the command buffer when executing kernel commands such as
`RUN_KERNEL_SLICE`.

The `packed_args` argument is populated from the memory range described in the
`CMP_KARGS_INFO` and `CMP_KUB_DESC` registers while the `ktb` argument is set to
an area of memory allocated by the CMP in TCDM based on the `CMP_TSD_INFO` and
`CMP_KUB_DESC` registers.

+--------+-------------------+-------------------------------------------------+
| Bits   | Field             | Description                                     |
+========+===================+=================================================+
| 31-0   | ENTRY_POINT_ADDR  | Address of the kernel entry point in memory.    |
+--------+-------------------+-------------------------------------------------+
| 63-32  | Reserved          | Reserved.                                       |
+--------+-------------------+-------------------------------------------------+

CMP_KUB_DESC (ID: 2)
^^^^^^^^^^^^^^^^^^^^

The `CMP_KUB_DESC` register describes where the Kernel Uniform Block is located
in device memory. The KUB contains data which is uniform to all kernel
instances, such as kernel arguments or the scheduling info template. It can be
read by accelerator cores but not written to.

When `KUB_SIZE` is set to zero, the KUB is not used when executing kernels using
the `RUN_KERNEL_SLICE` command. As a result, the `packed_args` and `ktd`
parameters are set to an invalid address when the entry point function is
invoked. This register is cleared upon reset of the RefSi platform, with all
fields set to zero.

Note: this is only used by the `RUN_KERNEL_SLICE` command.

+--------+-----------+-------------------------------------------------------------------+
| Bits   | Field     | Description                                                       |
+========+===========+===================================================================+
| 47-0   | KUB_ADDR  | Address of the Kernel Uniform Block.                              |
+--------+-----------+-------------------------------------------------------------------+
| 63-48  | KUB_SIZE  | Size of the Kernel Uniform Block in 256-byte increments.          |
+--------+-----------+-------------------------------------------------------------------+

CMP_KARGS_INFO (ID: 3)
^^^^^^^^^^^^^^^^^^^^^^

The `CMP_KARGS_INFO` register describes where packed kernel arguments are
located in device memory and must be set before executing any `RUN_KERNEL_SLICE`
command that makes use of kernel arguments.

When `KARGS_SIZE` is set to zero, the entry point function is invoked with an
invalid address for the the `packed_args` parameter. This register is cleared
upon reset of the RefSi platform, with all fields set to zero.

Note: this is only used by the `RUN_KERNEL_SLICE` command.

+--------+------------------+------------------------------------------------------------+
| Bits   | Field            | Description                                                |
+========+==================+============================================================+
| 15-0   | DUMMY            | Reserved.                                                  |
+--------+------------------+------------------------------------------------------------+
| 39-16  | KARGS_OFFSET     | Offset into the KUB where kernel argumenmts are stored.    |
+--------+------------------+------------------------------------------------------------+
| 63-40  | KARGS_SIZE       | Size of the packed kernel argument list.                   |
+--------+------------------+------------------------------------------------------------+

CMP_TSD_INFO (ID: 4)
^^^^^^^^^^^^^^^^^^^^

The `CMP_TSD_INFO` register describes where thread-specific data is located in
device memory. When a kernel is executed, the CMP copies thread-specific data to
TCDM in several locations. Each hardware thread executing the kernel has a
separate copy of the data (called the Kernel Thread Block or KTB), which can be
freely modified by the thread.

When `TSD_SIZE` is set to zero, the entry point function is invoked with an
invalid address for the `ktd` parameter and the KTB feature is not used. This
register is cleared upon reset of the RefSi platform, with all fields set to
zero.

One use-case for TSD is to let the CMP populate per-thread scheduling info in
the KTB (located in TCDM) from a single scheduling info template stored in the
KUB. Upon executing the kernel entry point, the `instance_id` and `slice_id`
parameters can be used to update scheduling info fields such as `group_id` and
`local_id`.

+--------+------------------+------------------------------------------------------------+
| Bits   | Field            | Description                                                |
+========+==================+============================================================+
| 15-0   | DUMMY            | Reserved.                                                  |
+--------+------------------+------------------------------------------------------------+
| 39-16  | TSD_OFFSET       | Offset into the KUB where thread-specific data is stored.  |
+--------+------------------+------------------------------------------------------------+
| 63-40  | TSD_SIZE         | Size of thread-specific data for the kernel.               |
+--------+------------------+------------------------------------------------------------+

CMP_STACK_TOP Register (ID: 5)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `CMP_STACK_TOP` register can be used to specify the address harts will use
as the top of the stack when executing the kernel entry point function. Since 
this address will be the same for all harts, it is recommended to set up a
memory window to enable each hart to have its own stack area.

+--------+-------------------+-------------------------------------------------+
| Bits   | Field             | Description                                     |
+========+===================+=================================================+
| 63-0   | STACK_TOP         | Address to use as the top of the stack.         |
+--------+-------------------+-------------------------------------------------+

CMP_RETURN_ADDR Register (ID: 6)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `CMP_RETURN_ADDR` register is used to specify the address to jump to when
returning from a kernel entry point function. It can be used to execute any
clean up code and notify the host when the execution of the kernel has
completed.

+--------+-------------------+---------------------------------------------------------+
| Bits   | Field             | Description                                             |
+========+===================+=========================================================+
| 63-0   | RETURN_ADDR       | Address to jump to when returning from a kernel.        |
+--------+-------------------+---------------------------------------------------------+

CMP_REG_WINDOW_BASEn (IDs: 8 through 15)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `CMP_REG_WINDOW_BASE` register array describes the base address of a memory
window, which is the starting address of a virtual memory area. Any accesses to
the virtual memory area described by the window actually occur in the target
memory area of the window.

+--------+------------------+------------------------------------------------------------+
| Bits   | Field            | Description                                                |
+========+==================+============================================================+
| 63-0   | BASE_ADDR        | Base address for the memory window.                        |
+--------+------------------+------------------------------------------------------------+

CMP_REG_WINDOW_TARGETn (IDs: 16 through 23)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `CMP_REG_WINDOW_TARGET` register array describes the target address of a memory
window, which is the starting address of a physical memory area. Any accesses to
the virtual memory area described by the window actually occur in the target
memory area of the window.

+--------+------------------+------------------------------------------------------------+
| Bits   | Field            | Description                                                |
+========+==================+============================================================+
| 63-0   | TARGET_ADDR      | Target address for the memory window.                      |
+--------+------------------+------------------------------------------------------------+

CMP_REG_WINDOW_MODEn (IDs: 24 through 31)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `CMP_REG_WINDOW_TARGET` register array contains most of the configuration
options for a given memory window. The most important field in this register is
the `ACTIVE` field, which controls whether or not the virtual memory area
described by the window is accessible. Any accesses to the virtual memory area
described by the window actually occur in the target memory area of the window.

The calculation of the effective address depends on the windowing mode:

* ``SHARED``: An access to ``BASE_ADRR + offset`` is mapped to
  ``TARGET + offset``. All harts and all cores see the same memory contents when
  accessing memory through the window.
* ``PER_HART``: An access to ``BASE_ADRR + offset`` is mapped to
  ``TARGET + (SCALE * hart_id) + offset``. Different harts see different memory
  contents when accessing memory through the window.
* ``PER_CORE``: An access to ``BASE_ADRR + offset`` is mapped to
  ``TARGET + (SCALE * core_id) + offset``. Different cores see different memory
  contents when accessing memory through the window.

In the above calculations, ``SCALE`` is substituted as ``SCALE_A * SCALE_B``. 
``SCALE_A`` and ``SCALE_B`` are configured through the ``CMP_REG_WINDOW_SCALE``
register array.

+--------+------------------+------------------------------------------------------------+
| Bits   | Field            | Description                                                |
+========+==================+============================================================+
| 0      | ACTIVE           | Whether or not the memory window can be accessed.          |
+--------+------------------+------------------------------------------------------------+
| 2-1    | MODE             | 0: SHARED, 1: PER_HART, 2: PER_CORE, 3: reserved           |
+--------+------------------+------------------------------------------------------------+
| 3      | INTERLEAVE       | Whether or not the target memory is interleaved.           |
+--------+------------------+------------------------------------------------------------+
| 6-4    | PERMS            | Memory permissions. [0]: READ, [1]: WRITE, [2]: EXECUTE    |
+--------+------------------+------------------------------------------------------------+
| 12-8   | STRIDE           | Granularity of the target area when it is interleaved.     |
+--------+------------------+------------------------------------------------------------+
| 63-32  | SIZE             | Size of the memory window, between 1 and 2^32 bytes.       |
+--------+------------------+------------------------------------------------------------+

CMP_REG_WINDOW_SCALEn (IDs: 32 through 39)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `CMP_REG_WINDOW_SCALE` register array describes the scaling factor to use
for calculating the effective address when accessing memory through a window
which is in per-hart or per-core mode. The scaling factor is the product of two
parts, ``SCALE_A`` and ``SCALE_B``.

+--------+------------------+------------------------------------------------------------+
| Bits   | Field            | Description                                                |
+========+==================+============================================================+
| 4-0    | SCALE_A          | First part of the scaling factor (0, 2^1, 2^2, ..., 2^31). |
+--------+------------------+------------------------------------------------------------+
| 63-32  | SCALE_B          | Second part of the scaling factor (1 .. 2^32).             |
+--------+------------------+------------------------------------------------------------+
