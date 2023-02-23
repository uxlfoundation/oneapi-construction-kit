Executing a Simple Command Buffer
=================================

The RefSi driver does not provide any function that executes a kernel function
on the device. Instead, it provides the ``refsiExecuteCommandBuffer`` function
which can be used to execute a series of simple commands on the RefSi command
processor (CMP), one of which invokes the same function on all the RISC-V
hardware threads (harts) contained in the RefSi device. As a result,
``kernel_exec`` needs to translate each 'kernel execution' operation into a
series of CMP commands that, executed together, result in the kernel being
computed in parallel on the RefSi device.

In order to demonstrate this approach, we will start with executing a very
simple command buffer that writes the address of the kernel entry point function
to the relevant CMP register (``CMP_REG_ENTRY_PT_FN``) and finishes executing.
Running the ``hello`` example should output CMP debug messages that shows a
command buffer containing two commands, ``WRITE_REG64`` and ``FINISH``, being
executed on the device.

The first part to executing a command buffer is to encode the individual
commands. The command buffer format is based on a list of 64-bit chunks. The
``refsiEncodeCMPCommand`` driver function is used to encode the first ('header')
chunk of a command. To simplify encoding the same command multiple times, we add
a utility class (``refsi_command_buffer``) that contains functions to encode
RefSi commands.

A new header file ``refsi_command_buffer.h``, needs to be created first:

.. code:: c++

    // include/refsi_command_buffer.h
    #ifndef _HAL_REFSI_TUTORIAL_REFSI_COMMAND_BUFFER_H
    #define _HAL_REFSI_TUTORIAL_REFSI_COMMAND_BUFFER_H

    #include <vector>

    #include "refsidrv/refsidrv.h"
    #include "refsi_hal.h"

    class refsi_hal_device;

    /// @brief Utility class that can be used to generate RefSi command buffers and
    /// execute them on a RefSi device.
    class refsi_command_buffer {
     public:
      /// @brief Add a command to stop execution of commands in the command buffer.
      void addFINISH();

      /// @brief Add a command to store a 64-bit value to a CMP register.
      /// @param reg CMP register index to write to.
      /// @param value Immediate value to write to the register.
      void addWRITE_REG64(refsi_cmp_register_id reg, uint64_t value);

      /// @brief Execute the commands that have been added to the buffer.
      /// @param hal_device Device to execute the command buffer.
      refsi_result run(refsi_hal_device &hal_device);

    private:
      std::vector<uint64_t> chunks;
    };

    #endif  // _HAL_REFSI_TUTORIAL_REFSI_COMMAND_BUFFER_H

A new source file, ``refsi_command_buffer.cpp``, needs to be created as well to
match the header shown just before. It defines functions to encode ``FINISH``
and ``WRITE_REG64`` commands:

.. code:: c++

    // source/refsi_command_buffer.cpp
    #include "refsi_command_buffer.h"
    #include "refsi_hal.h"

    void refsi_command_buffer::addFINISH() {
      chunks.push_back(refsiEncodeCMPCommand(CMP_FINISH, 0, 0));
    }

    void refsi_command_buffer::addWRITE_REG64(refsi_cmp_register_id reg,
                                              uint64_t value) {
      chunks.push_back(refsiEncodeCMPCommand(CMP_WRITE_REG64, 1, reg));
      chunks.push_back(value);
    }

.. code:: c++

    // refsi_hal.h
    #include <vector>       // Added
    ...
    class refsi_hal_device : public hal::hal_device_t {
      ...
     private:
      void addFINISH(std::vector<uint64_t> &chunks);
      void addWRITE_REG64(std::vector<uint64_t> &chunks, refsi_cmp_register_id reg,
                          uint64_t value);
    };

Finally, the new source file needs to be registered with CMake so that it will
be built:

.. code::

  # source/CMakeLists.txt

  add_library(hal_refsi_tutorial SHARED
    hal_main.cpp
    refsi_hal.cpp
    refsi_command_buffer.cpp # Added
  )

This utility class can make creating a command buffer very simple:

.. code:: c++

    refsi_command_buffer cb;
    cb.addWRITE_REG64(CMP_REG_ENTRY_PT_FN, kernel_address);
    cb.addFINISH();

The simplest command buffer would contain a single ``FINISH`` command and no
other commands. We have chosen to include an extra ``WRITE_REG64`` command so
that the command buffer performs an operation on the RefSi device. However,
writing to the ``CMP_REG_ENTRY_PT_FN`` register, while changing the value in
this register, has no functional effect unless a ``RUN_KERNEL_SLICE`` command
is used later. This will be done in the next section.

Since the CMP can only execute command buffers that are located in device
memory, we need to allocate device memory for the commands and then
write the command buffer chunks to the allocated memory. This can be done using
previously-implemented HAL operations ``mem_alloc`` and ``mem_write``:

.. code:: c++

    size_t cb_size = cb_chunks.size() * sizeof(uint64_t);
    hal::hal_addr_t cb_addr = mem_alloc(cb_size, sizeof(uint64_t));
    mem_write(cb_addr, cb_chunks.data(), cb_size);

Finally, the command can be executed. The ``refsiExecuteCommandBuffer`` is
asynchronous, which means it does not wait for the command buffer to have
finished executed before returning. The ``refsiWaitForDeviceIdle`` function can
be used for that purpose:

.. code:: c++

  refsiExecuteCommandBuffer(device, cb_addr, cb_size);
  refsiWaitForDeviceIdle(device);

Putting everything together and adding error handling, here is the code for
``kernel_exec`` at the end of this sub-step:

.. code:: c++

    // refsi_hal.cpp
    #include "refsi_command_buffer.h" // Added

    bool refsi_hal_device::kernel_exec(hal::hal_program_t program,
                                       hal::hal_kernel_t kernel,
                                       const hal::hal_ndrange_t *nd_range,
                                       const hal::hal_arg_t *args,
                                       uint32_t num_args, uint32_t work_dim) {
      refsi_locker locker(hal_lock);

      refsi_hal_kernel *kernel_wrapper = (refsi_hal_kernel *)kernel;

      // Encode the command buffer.
      refsi_command_buffer cb;
      cb.addWRITE_REG64(CMP_REG_ENTRY_PT_FN, kernel_wrapper->symbol);
      cb.addFINISH();

      // Execute the command buffer.
      if (refsi_success != cb.run(*this)) {
        return false;
      }

      return true;
    }

A ``run`` function also needs to be added to ``refsi_command_buffer``, which
handles the execution of a command buffer on the RefSi device:

.. code:: c++

    // source/refsi_command_buffer.cpp
    refsi_result refsi_command_buffer::run(refsi_hal_device &hal_device) {
      // Write the command buffer to device memory.
      size_t cb_size = chunks.size() * sizeof(uint64_t);
      hal::hal_addr_t cb_addr = hal_device.mem_alloc(cb_size, sizeof(uint64_t));
      if (!cb_addr || !hal_device.mem_write(cb_addr, chunks.data(), cb_size)) {
        return refsi_failure;
      }

      // Execute the command buffer and wait for its completion.
      if (refsi_result result = refsiExecuteCommandBuffer(hal_device.get_device(),
                                                          cb_addr, cb_size)) {
        hal_device.mem_free(cb_addr);
        return result;
      }
      refsiWaitForDeviceIdle(hal_device.get_device());
      hal_device.mem_free(cb_addr);
      return refsi_success;
    }

Running the ``hello`` example, we can see the two commands being executed on the
CMP as well the values passed to the commands (e.g. the kernel entry point
address is ``0x1038a``):

.. code:: console

    $ REFSI_DEBUG=cmp bin/hello
    Using device 'RefSi M1 Tutorial'
    Running hello example (Global size: 8, local size: 1)
    [CMP] Starting.
    [CMP] Starting to execute command buffer at 0xbfffffe8.
    [CMP] CMP_WRITE_REG64(ENTRY_PT_FN, 0x1001c)
    [CMP] CMP_FINISH
    [CMP] Finished executing command buffer in 0.000 s
    [CMP] Requesting stop.
    [CMP] Stopping.

While the example no longer reports any error, it also does not produce the
expected output, which is a series of messages like the following:

.. code:: console

    Hello from clik_sync! tid=0, lid=0, gid=0
    Hello from clik_sync! tid=1, lid=0, gid=1
    Hello from clik_sync! tid=2, lid=0, gid=2
    Hello from clik_sync! tid=3, lid=0, gid=3
    ...

This is because the command buffer we are executing does not invoke the kernel
function on the RISC-V cores. Doing so involves the ``CMP_RUN_KERNEL_SLICE``
command, which will be presented in the next section.

