RefSi Driver Interface
======================

The RefSi virtual device can be controlled by the RefSi device driver, which
mimics how a real accelerator would be exposed to the system. The driver has a
C API and can be used to manage memory transfers between the host and simulated
device as well as enqueue commands (work) onto the device.

The RefSi driver currently supports two families of accelerators, RefSi M and
RefSi G. While there are significant differences to how kernels can be executed
on each kind of accelerator, the driver API is largely the same for both
families. Functions that can be only used on one or the other will be marked as
such.

Driver start-up and teardown
----------------------------

The ``refsiInitialize`` function must be called prior to calling any other
driver function. It allocated global resources needed to manage the virtual
RefSi device. These resources can and should be freed by calling the
``refsiTerminate`` function prior to the process unloading.

.. code:: c++

    /// @brief Initialize the driver. No other driver function can be called prior
    /// to calling this function.
    REFSI_API refsi_result refsiInitialize();

    /// @brief Terminate the driver. No driver function other than refsiInitialize
    /// can be called after calling this function.
    REFSI_API refsi_result refsiTerminate();

Once ``refsiInitialize`` has been called, the ``refsiOpenDevice`` function can
be used to establish a connection to a virtual RefSi device where the device
family is passed as an argument, returning a handle to the device. Calling this
function multiple times with the same family returns the same handle; multiple
devices are not created.

``refsiShutdownDevice`` can be used to stop a RefSi device using its handle and
reclaim its resources. Calling ``refsiOpenDevice`` then results in a new device
being created.

.. code:: c++

    /// @brief Represents the kind of RefSi device to control.
    enum refsi_device_family {
      REFSI_DEFAULT = 0,
      REFSI_M = 1,
      REFSI_G = 2
    };

    /// @brief Open the device. This establishes a connection with the device and
    /// ensures that it has been successfully started. Device memory functions, as
    /// well as command buffer execution functions, can only be called after this
    /// function has been called.
    /// @param family Type of RefSi device to open a connection to.
    REFSI_API refsi_device_t refsiOpenDevice(refsi_device_family family);

    /// @brief Shut down the device. Any pointer returned by refsiGetMappedAddress
    /// can no longer be used after this function is called.
    REFSI_API refsi_result refsiShutdownDevice(refsi_device_t device);

Device queries
--------------

Not unlike real platforms, RefSi virtual devices can have different
configurations that can be characterized by a number of metrics such as number
of cores and hardware threads, vector register size, ISA extensions, memory size
and so on. The ``refsiQueryDeviceInfo`` function can be used to query a RefSi
device for a number of these characteristics. It is highly recommended to use
this function rather than hard-coding device metrics, since they can change
through build-time configuration or newer RefSi driver releases.

.. code:: c++

    /// @brief Provides information about the device.
    typedef struct refsi_device_info {
      /// @brief Kind of device.
      refsi_device_family family;
      /// @brief Number of accelerator cores contained within the device.
      unsigned num_cores;
      /// @brief Number of hardware threads contained in each accelerator core.
      unsigned num_harts_per_core;
      /// @brief Number of entries in the device's memory map.
      unsigned num_memory_map_entries;
      /// @brief String that describes the ISA exposed by the cores.
      const char *core_isa;
      /// @brief Width of the cores' vector registers, in bits.
      unsigned core_vlen;
      /// @brief Maximum width of an element in a vector register, in bits.
      unsigned core_elen;
    } refsi_device_info_t;

    /// @brief Query information about the device.
    /// @param device Device to query information for.
    /// @param device_info To be filled with information about the device.
    REFSI_API refsi_result refsiQueryDeviceInfo(refsi_device_t device,
                                                refsi_device_info_t *device_info);

Another aspect of RefSi configuration that can be queried through the RefSi
driver API is the device's memory map. The map is simply a list of entries,
where each entry a kind of memory, its size and the memory address it is mapped
at. This includes both 'conventional' memory kinds like DRAM and TCM as well as
memory-like regions such as memory-mapped registers (e.g. DMA and performance
counter registers). The number of entries in this map can be found in the
``num_memory_map_entries`` entries of the struct returned by
``refsiQueryDeviceInfo``.

.. code:: c++

    enum refsi_memory_map_kind {
      /// @brief The kind of memory for this memory map entry is unknown.
      UNKNOWN = 0,
      /// @brief Refers to the area of memory where the device's dedicated memory is
      /// mapped. DRAM is shared between all device cores.
      DRAM = 1,
      /// @brief Refers to the area of memory where the device's entire
      /// tightly-coupled instruction memory is mapped, for all cores.
      TCIM = 2,
      /// @brief Refers to the area of memory where the device's entire
      /// tightly-coupled data memory is mapped, for all cores.
      TCDM = 3,
      /// @brief Refers to the area of memory where each core's tightly-coupled data
      /// memory is mapped. This range has the same address for all cores, however
      /// each core will see different contents when accessing it.
      TCDM_PRIVATE = 4,
      /// @brief Refers to the area of memory where Kernel DMA registers are mapped
      /// for all hardware threads.
      KERNEL_DMA = 5,
      /// @brief Refers to the area of memory where a hardware thread's Kernel DMA
      /// registers are mapped. This range has the same address for all hardware
      /// threads, however each hart will see different contents when accessing it.
      KERNEL_DMA_PRIVATE = 6,
      /// @brief Refers to the area of memory where Performance Counter registers
      /// are mapped. This is divided into a per-hardware-thread area and a global
      /// area shared between all units in the RefSi device..
      PERF_COUNTERS = 7,
    };

    /// @brief Represents an entry in the device's memory map.
    struct refsi_memory_map_entry {
      /// @brief Kind of memory this memory range refers to.
      refsi_memory_map_kind kind;
      /// @brief Starting address of the memory range in device memory.
      refsi_addr_t start_addr;
      /// @brief Size of the memory range in device memory, in bytes.
      size_t size;
    };

    /// @brief Query an entry in the device's memory map.
    /// @param device Device to query memory map info for.
    /// @param index Index of the entry to query.
    /// @param entry To be filled with information about the memory map entry.
    REFSI_API refsi_result refsiQueryDeviceMemoryMap(refsi_device_t device,
                                                     size_t index,
                                                     refsi_memory_map_entry *entry);

Device memory allocation
------------------------

In order to run programs (kernels) on an accelerator, some kind of device
memory is typically required. The ``refsiAllocDeviceMemory`` and
``refsiFreeDeviceMemory`` can be used to allocate and free memory for a
particular RefSi device. The kind ``argument`` can be used to specify where the
memory should be allocated, however only ``DRAM`` allocations are currently
guaranteed to be supported by the device.

.. code:: c++

    /// @brief Allocate device memory.
    /// @param device Device to allocate memory on.
    /// @param size Size of the memory range to allocate, in bytes.
    /// @param alignment Minimum alignment for the returned physical address.
    /// @param kind Kind of memory to allocate, e.g. DRAM, scratchpad.
    REFSI_API refsi_addr_t refsiAllocDeviceMemory(refsi_device_t device,
                                                  size_t size, size_t alignment,
                                                  refsi_memory_map_kind kind);

    /// @brief Free device memory allocated with refsiAllocDeviceMemory.
    /// @param device Device to free memory from.
    /// @param phys_addr Device address to free.
    REFSI_API refsi_result refsiFreeDeviceMemory(refsi_device_t device,
                                                 refsi_addr_t phys_addr);


Device memory access
--------------------

Device memory allocated through ``refsiAllocDeviceMemory`` can be accessed
through the host (i.e. using the RefSi driver interface) in one of two ways.

The first way is through ``memcpy``-like functions that can either read from
or write to device memory, ``refsiReadDeviceMemory`` and
``refsiWriteDeviceMemory``. These functions are blocking and only return once
the operation has been completed. There is also no need to manually control the
device's cache(s).

Note how the ``unit_id`` parameter can be used to target different kinds of
memory. For example, different harts can access a particular area of TCDM at the
same address while seeing different contents (i.e. there is one copy of this
area for each hart). Passing a hart ID as ``unit_id`` allows for accessing
hart-local storage for that specific hart.

.. code:: c++

    /// @brief Read data from device memory.
    /// @param device Device to read from.
    /// @param dest Buffer to copy read data to.
    /// @param phys_addr Device address that defines the start of the memory range
    /// to read from.
    /// @param size Size of the memory range to read, in bytes.
    /// @param unit_id UnitID of the execution unit to use when making memory
    /// requests. This is usually 'external' but hart IDs can also be used.
    REFSI_API refsi_result refsiReadDeviceMemory(refsi_device_t device,
                                                 uint8_t *dest,
                                                 refsi_addr_t phys_addr,
                                                 size_t size, uint32_t unit_id);

    /// @brief Write data to device memory.
    /// @param device Device to write to.
    /// @param phys_addr Device address that defines the start of the memory range
    /// to write to.
    /// @param source Buffer that contains the data to write to device memory.
    /// @param size Size of the memory range to write, in bytes.
    /// @param unit_id UnitID of the execution unit to use when making memory
    /// requests. This is usually 'external' but hart IDs can also be used.
    REFSI_API refsi_result refsiWriteDeviceMemory(refsi_device_t device,
                                                  refsi_addr_t phys_addr,
                                                  const uint8_t *source,
                                                  size_t size, uint32_t unit_id);

The second way of accessing device memory from the host is through memory
mapping. The RefSi driver maps all device memory on the host, so that a host
pointer can be used to access it. The ``refsiGetMappedAddress`` function can be
used to retrieve a pointer to a specific device memory address. With this
pointer, data can be transparently copied to and from the device using functions
like ``memcpy``.

The ``refsiFlushDeviceMemory`` and ``refsiInvalidateDeviceMemory`` are meant
for controlling the caches between the host and the device. The former should be
used after writing to a pointer returned from ``refsiGetMappedAddress`` while
the latter should be used after the device has potentially written to device
memory (e.g. by executing a kernel) prior to reading from mapped memory.

.. code:: c++

    /// @brief Get a CPU-accessible pointer that maps to the given device address.
    /// Device memory is first mapped when a connection to the device is established
    /// and unmapped when the connection is closed.
    /// @param device Device to retrieve a mapped pointer for.
    /// @param phys_addr Device address to map to a virtual address (CPU pointer).
    /// @param size Size of the memory range to access, in bytes.
    REFSI_API void *refsiGetMappedAddress(refsi_device_t device,
                                          refsi_addr_t phys_addr, size_t size);

    /// @brief Flush any changes to device memory from the CPU cache.
    /// @param device Device to flush data changes to.
    /// @param phys_addr Device address that defines the start of the memory range
    /// to flush from the CPU cache.
    /// @param size Size of the memory range to flush, in bytes.
    REFSI_API refsi_result refsiFlushDeviceMemory(refsi_device_t device,
                                                  refsi_addr_t phys_addr,
                                                  size_t size);

    /// @brief Invalidate any cached device data from the CPU cache.
    /// @param device Device to flush data changes from.
    /// @param phys_addr Device address that defines the start of the memory range
    /// to invalidate from the CPU cache.
    /// @param size Size of the memory range to invalidate, in bytes.
    REFSI_API refsi_result refsiInvalidateDeviceMemory(refsi_device_t device,
                                                       refsi_addr_t phys_addr,
                                                       size_t size);

Device execution (RefSi M1)
---------------------------

On RefSi M1, executing work is done through the command processor (CMP). The
CMP executes command buffers, which are simply lists of commands that are stored
in device memory.

Executing command buffers
^^^^^^^^^^^^^^^^^^^^^^^^^

A command buffer can be enqueued on the device from the host using the
``refsiExecuteCommandBuffer`` function by passing a device address and size of
the command buffer. This function is asynchronous and will likely return before
the commands have finished executing. ``refsiWaitForDeviceIdle`` can be used to
block the current host thread until previously enqueued commands have been
executed.

.. code:: c++

    /// @brief Asynchronously execute a series of commands on the device.
    /// @param device Device to execute a command buffer on.
    /// @param cb_addr Address of the command buffer in device memory.
    /// @param size Size of the command buffer, in bytes.
    REFSI_API refsi_result refsiExecuteCommandBuffer(refsi_device_t device,
                                                     refsi_addr_t cb_addr,
                                                     size_t size);

    /// @brief Wait for all previously enqueued command buffers to be finished.
    /// @param device Device to wait for.
    REFSI_API void refsiWaitForDeviceIdle(refsi_device_t device);

Encoding command buffers
^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: c++

    /// @brief Identifies a command that can be executed by the command processor.
    enum refsi_cmp_command_id {
      CMP_NOP = 0,
      CMP_FINISH = 1,
      CMP_WRITE_REG64 = 2,
      CMP_LOAD_REG64 = 3,
      CMP_STORE_REG64 = 4,
      CMP_STORE_IMM64 = 5,
      CMP_COPY_MEM64 = 6,
      CMP_RUN_KERNEL_SLICE = 7,
      CMP_RUN_INSTANCES = 8,
      CMP_SYNC_CACHE = 9
    };

    /// @brief Encode a CMP command header.
    /// @param opcode Command's opcode to encode.
    /// @param chunk_count Command's chunk count to encode.
    /// @param inline_chunk Command's inline chunk to encode.
    REFSI_API uint64_t refsiEncodeCMPCommand(refsi_cmp_command_id opcode,
                                             uint32_t chunk_count,
                                             uint32_t inline_chunk);

    /// @brief Try to decode a CMP command header.
    /// @param opcode Populated with the command's opcode.
    /// @param chunk_count Populated with the command's chunk count.
    /// @param inline_chunk Populated with the command's inline chunk.
    REFSI_API refsi_result refsiDecodeCMPCommand(uint64_t header,
                                                 refsi_cmp_command_id *opcode,
                                                 uint32_t *chunk_count,
                                                 uint32_t *inline_chunk);

Device execution (RefSi G1)
---------------------------

Execution of kernels differs between RefSi G1 and RefSi M1. Unlike M1, G1 does
not feature a command processor (CMP) and as such the
``refsiExecuteCommandBuffer`` command cannot be used to enqueue work onto the
device. Executing a kernel can be done using the ``refsiExecuteKernel``, which
boots the RISC-V cores on the given number of harts. A RISC-V bootloader is
typically used to invoke the kernel.

.. code:: c++

    /// @brief Synchronously execute a kernel on the device. Only supported on RefSi
    /// G1 devices.
    REFSI_API refsi_result refsiExecuteKernel(refsi_device_t device,
                                              refsi_addr_t entry_fn_addr,
                                              uint32_t num_harts);
