Loading and Executing Kernels Without Scheduling
================================================

Now that we can execute a simple command buffer on the CMP, it is time to extend
the command buffer to execute the kernel function using the
``CMP_RUN_KERNEL_SLICE`` command. To keep things simple, we will only use a
single hardware thread to execute kernels and focus on the ``hello`` example
with a local size of one item.

Loading Kernels
^^^^^^^^^^^^^^^

Before kernels can be executed on the RefSi device, the executable object (in
this case, an ELF object) containing the kernel needs to be loaded in the
device's memory. This is done using a simple ELF loader provided for this
tutorial.

In order to simplify the loading of kernels on the device, this tutorial uses a
linker script which lays out the kernel code at a fixed address in memory when
compiling kernels to RISC-V executable objects. The linker script file can be
found at ``hal_refsi_tutorial/include/device/program.lds``. The area of memory
used to store kernel code starts at address ``0x10000`` and ends at ``0xfffff``:

.. code:: c++

    // refsi_hal.cpp
    ...
    constexpr const uint64_t REFSI_ELF_BASE = 0x10000ull;                  // Added
    constexpr const uint64_t REFSI_ELF_SIZE = (1 << 20) - REFSI_ELF_BASE;  // Added

The RefSi M1 device we are targetting in this tutorial does not have any memory
mapped at ``[0x10000:0xfffff]``, which requires a memory mapping to be created
before the ELF object can be loaded onto the device. Such memory mappings can be
created using a RefSi feature called memory window, which sets up the device to
redirect memory accesses from one region of memory to another.

Creating a memory window involves running a command buffer that writes to
several CMP registers that control the configuration of a memory window. In
order to simplify this process, we create a utility function that takes care of
populating a command buffer with the required register writes:

.. code:: c++

    // refsi_hal.h
    ...
    class refsi_command_buffer; // Added
    ...
    class refsi_hal_device : public hal::hal_device_t {
     private:
      bool createWindow(refsi_command_buffer &cb, uint32_t win_id,              // Added
                        uint32_t mode, refsi_addr_t base, refsi_addr_t target,  // Added
                        uint64_t scale, uint64_t size);                         // Added
    };

.. code:: c++

    // refsi_hal.cpp
    bool refsi_hal_device::createWindow(refsi_command_buffer &cb,
                                        uint32_t win_id, uint32_t mode,
                                        refsi_addr_t base, refsi_addr_t target,
                                        uint64_t scale, uint64_t size) {
      auto base_reg = (refsi_cmp_register_id)(CMP_REG_WINDOW_BASE0 + win_id);
      auto target_reg = (refsi_cmp_register_id)(CMP_REG_WINDOW_TARGET0 + win_id);
      auto mode_reg = (refsi_cmp_register_id)(CMP_REG_WINDOW_MODE0 + win_id);
      auto scale_reg = (refsi_cmp_register_id)(CMP_REG_WINDOW_SCALE0 + win_id);

      // Break down the scale into two factors, a and b.
      if (scale > (1ull << 32)) {
        return false;
      }
      uint64_t scale_a = (scale > 0) ? 1 : 0;
      uint64_t scale_b = (scale > 0) ? scale - 1 : 0;

      // Encode the mode register value.
      uint64_t mode_val = CMP_WINDOW_ACTIVE | (mode & 0x6) |
          ((size - 1) & 0xffffffff) << 32ull;

      // Add register writes to the command buffer.
      cb.addWRITE_REG64(base_reg, base);
      cb.addWRITE_REG64(target_reg, target);
      cb.addWRITE_REG64(scale_reg, (scale_a & 0x1f) | (scale_b << 32ull));
      cb.addWRITE_REG64(mode_reg, mode_val);
      return true;
    }

In this tutorial we first allocate a buffer in DRAM (using ``mem_alloc`` we have
implemented earlier) for the ELF executable and then create a memory window that
maps the ``[0x10000:0xfffff]`` region to the DRAM buffer using the utility
function we just added:

.. code:: c++

    // refsi_hal.h
    class refsi_hal_device : public hal::hal_device_t {
     private:
      bool createWindows();                     // Added
      ...
      hal::hal_addr_t elf_mem_base = 0;         // Added
      hal::hal_addr_t elf_mem_size = 0;         // Added
      hal::hal_addr_t elf_mem_mapped_addr = 0;  // Added
    };

.. code:: c++

    // refsi_hal.cpp
    bool refsi_hal_device::createWindows() {
      refsi_command_buffer cb;

      // Set up a memory window for ELF executables.
      // Allocate 'ELF' memory in DRAM, to store kernel executables.
      elf_mem_base = REFSI_ELF_BASE;
      elf_mem_size = REFSI_ELF_SIZE;
      if (elf_mem_mapped_addr) {
        mem_free(elf_mem_mapped_addr);
      }
      elf_mem_mapped_addr = mem_alloc(elf_mem_size, 4096);
      if (!elf_mem_mapped_addr) {
        return false;
      }
      if (!createWindow(cb, 0 /* win_id */, CMP_WINDOW_MODE_SHARED, elf_mem_base,
                        elf_mem_mapped_addr, 0, elf_mem_size)) {
        return false;
      }

      cb.addFINISH();
      return cb.run(*this) == refsi_success;
    }

Finally, we need to call ``createWindows`` when first initializing the device.
The memory window only needs to be created once, and not before executing each
kernel on the device. This will be done in the ``initialize`` function:

.. code:: c++

    // refsi_hal.cpp
    bool refsi_hal_device::initialize() {
      if (!createWindows()) { // Added
        return false;         // Added
      }                       // Added
      return true;
    }

There is one final step needed before the ELF can be loaded. We need to add a
new utility class, ``hal_mem_device``. This class inherits from the
``MemoryDeviceBase`` base class, which abstracts devices with memory that can be
written to and read from:

.. code:: c++

    // refsi_hal.h
    #include "common_devices.h" // Added
    ...
    class hal_mem_device : public MemoryDeviceBase { // Added class
     public:
      hal_mem_device(hal::hal_device_t *hal_device) : hal_device(hal_device) {}

      /// @brief Return zero. Memory controllers are variable-sized devices.
      size_t mem_size() const override { return 0; }

      bool load(reg_t addr, size_t len, uint8_t *bytes, unit_id_t unit) override {
        return hal_device->mem_read(bytes, addr, len);
      }

      bool store(reg_t addr, size_t len, const uint8_t *bytes,
                 unit_id_t unit) override {
        return hal_device->mem_write(addr, bytes, len);
      }

     private:
      hal::hal_device_t *hal_device;
    };                                               // End class

Once this is done, the ELF loader (through the ``ELFProgram`` class) can copy
kernel code to the ``[0x10000:0xfffff]`` region without being aware of the
memory window and the data will automatically end up in DRAM instead. Note how
there is no mention of memory windows or the ELF base address when loading the
kernel's ELF:

.. code:: c++

    // refsi_hal.cpp
    bool refsi_hal_device::kernel_exec(hal::hal_program_t program,
                                       hal::hal_kernel_t kernel,
                                       const hal::hal_ndrange_t *nd_range,
                                       const hal::hal_arg_t *args,
                                       uint32_t num_args, uint32_t work_dim) {
      refsi_locker locker(hal_lock);

      refsi_hal_program *refsi_program = (refsi_hal_program *)program; // Added
      ELFProgram *elf = refsi_program->elf.get();                      // Added
      refsi_hal_kernel *kernel_wrapper = (refsi_hal_kernel *)kernel;

      // Load ELF into the RefSi device's memory.                      // Added
      hal_mem_device mem_if(this);                                     // Added
      if (!elf->load(mem_if)) {                                        // Added
        return false;                                                  // Added
      }                                                                // Added
      ...
    }

When executing the ``hello`` example again, we can see that a new command buffer
is executed when starting up the RefSi device. Writes to CMP registers are
present in the debug output and give an indication as to how the memory window
is set up (e.g. the base address being ``0x10000`` and the target address
that was allocated in DRAM to store ELF kernels being ``0xbff10000``):

.. code:: console

    $ REFSI_DEBUG=cmp bin/hello
    [CMP] Starting.
    [CMP] Starting to execute command buffer at 0xbff0ffb8.
    [CMP] CMP_WRITE_REG64(WINDOW_BASE0, 0x10000)
    [CMP] CMP_WRITE_REG64(WINDOW_TARGET0, 0xbff10000)
    [CMP] CMP_WRITE_REG64(WINDOW_SCALE0, 0x0)
    [CMP] CMP_WRITE_REG64(WINDOW_MODE0, 0xeffff00000001)
    [CMP] CMP_FINISH
    [CMP] Finished executing command buffer in 0.000 s
    Using device 'RefSi M1 Tutorial'
    Running hello example (Global size: 8, local size: 1)
    [CMP] Starting to execute command buffer at 0xbff0ffe8.
    [CMP] CMP_WRITE_REG64(ENTRY_PT_FN, 0x1001c)
    [CMP] CMP_FINISH
    [CMP] Finished executing command buffer in 0.000 s
    [CMP] Requesting stop.
    [CMP] Stopping.

However, we are still not seeing any of the expected output from the ``hello``
example. This is because none of the command buffers contain any commands that
execute code on the RefSi accelerator cores, such as ``CMP_RUN_KERNEL_SLICE``.
This will be addressed in the next section.

Executing Kernels
^^^^^^^^^^^^^^^^^

Now that the kernel executable has been loaded in the RefSi device's memory,
we can start looking at how to execute the kernel on the device.

First, we want to add a helper function to encode the ``CMP_RUN_KERNEL_SLICE``
command:

.. code:: c++

    // refsi_command_buffer.h
    class refsi_command_buffer {
      ...
     private:
       void addRUN_KERNEL_SLICE(uint32_t max_harts, uint64_t num_instances,
                                uint64_t slice_id);
    };

.. code:: c++

    // refsi_command_buffer.cpp
    void refsi_command_buffer::addRUN_KERNEL_SLICE(uint32_t max_harts,
                                                   uint64_t num_instances,
                                                   uint64_t slice_id) {
      uint32_t inline_chunk = (max_harts & 0xff);
      chunks.push_back(
          refsiEncodeCMPCommand(CMP_RUN_KERNEL_SLICE, 2, inline_chunk));
      chunks.push_back(num_instances);
      chunks.push_back(slice_id);
    }

This command has a few parameters:

* ``max_harts``: specifies the maximum number of (parallel) hardware threads to
  use for executing the kernel. We will set this value to one in order to have
  sequential execution.
* ``num_instances``: specifies how many times to invoke the kernel entry point
  function. This will be equal to the number of work-groups.
* ``slice_id``: identifies the slice command when it is used multiple times for
  the same kernel. This will be the case for 2D and 3D kernels.

When this command is executed, the kernel entry point function is invoked
multiple times on the given number of RISC-V hardware threads. The entry point
address is taken from the ``CMP_REG_ENTRY_PT_FN`` register, which is already set
in the command buffer. This address must be in device memory, which means that
the ELF program containing the kernel code must also be loaded on the device as
we have done in the previous section.

Second, we add a ``RUN_KERNEL_SLICE`` command to the command buffer that is
being populated in ``kernel_exec``. Building on the previous section, here is
that function looks now after adding the command to run the kernel:

.. code:: c++

    // refsi_hal.cpp
    bool refsi_hal_device::kernel_exec(hal::hal_program_t program,
                                       hal::hal_kernel_t kernel,
                                       const hal::hal_ndrange_t *nd_range,
                                       const hal::hal_arg_t *args,
                                       uint32_t num_args, uint32_t work_dim) {
      refsi_locker locker(hal_lock);

      ELFProgram *elf = (ELFProgram *)program;
      refsi_hal_kernel *kernel_wrapper = (refsi_hal_kernel *)kernel;

      // Load ELF into the RefSi device's memory.
      hal_mem_device mem_if(this);
      if (!elf->load(mem_if)) {
        return false;
      }

      // Encode the command buffer.
      size_t num_instances = nd_range->global[0] / nd_range->local[0];         // Added
      cb.addWRITE_REG64(CMP_REG_ENTRY_PT_FN, kernel_wrapper->symbol);
      cb.addWRITE_REG64(CMP_REG_RETURN_ADDR, elf->find_symbol("kernel_exit")); // Added
      cb.addRUN_KERNEL_SLICE(1, num_instances, 0);                             // Added
      cb.addFINISH();

      // Write the command buffer to device memory.
      ...
    }

When executing the ``hello`` example again, we can see that a
``CMP_RUN_KERNEL_SLICE`` command is executed and that an instruction is executed
on device (thanks to setting the ``SPIKE_SIM_LOG`` environment variable to print
a trace of simulated RISC-V instructions):

.. code:: console

    $ SPIKE_SIM_LOG=1 REFSI_DEBUG=cmp bin/hello
    [CMP] Starting.
    [CMP] Starting to execute command buffer at 0xbff0ffb8.
    [CMP] CMP_WRITE_REG64(WINDOW_BASE0, 0x10000)
    [CMP] CMP_WRITE_REG64(WINDOW_TARGET0, 0xbff10000)
    [CMP] CMP_WRITE_REG64(WINDOW_SCALE0, 0x0)
    [CMP] CMP_WRITE_REG64(WINDOW_MODE0, 0xeffff00000001)
    [CMP] CMP_FINISH
    [CMP] Finished executing command buffer in 0.000 s
    Using device 'RefSi M1 Tutorial'
    Running hello example (Global size: 8, local size: 1)
    [CMP] Starting to execute command buffer at 0xbff0ffd0.
    [CMP] CMP_WRITE_REG64(ENTRY_PT_FN, 0x1001c)
    [CMP] CMP_WRITE_REG64(RETURN_ADDR, 0x1006c)
    [CMP] CMP_RUN_KERNEL_SLICE(n=8, slice_id=0, max_harts=1)
    core   0: 0x000000000001001c (0x000066bc) c.ld    a5, 72(a3)
    core   0: exception trap_load_access_fault, epc 0x000000000001001c
    core   0:           tval 0x0000000000000048
    error: 'Load Access Fault' exception was raised @ 0x1001c (badaddr = 0x48)
    [CMP] Finished executing command buffer in 0.000 s
    [CMP] Requesting stop.
    [CMP] Stopping.

However, the kernel is seen to crash with a load access fault when executing the
first instruction. This is because the second argument to the entry point
function has not been initialized. We will see how to resolve this issue in the
next section.

