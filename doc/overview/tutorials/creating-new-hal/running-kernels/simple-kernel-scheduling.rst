Simple Kernel Scheduling
========================

To understand the issue shown in the previous section, let us have a look at the
required signature for the kernel entry point function:

.. code:: c++

    typedef struct {
      // Kernel argument 1
      // Kernel argument 2
      // ...
      // Kernel argument n
    } kernel_args;
    
    typedef struct wg_info {
      size_t group_id[3];
      size_t num_groups[3];
      size_t global_offset[3];
      size_t local_size[3];
      uint32_t num_dim;
      size_t num_groups_per_call[3];
      uintptr_t hal_extra;
    } wg_info_t;

    void kernel_main(uint64_t instance_id, uint64_t slice_id,
                     const kernel_args *args, wg_info_t *wg)

The third argument, ``args``, is not used by the ``hello`` example and we will
ignore it in this section. The fourth argument, ``wg``, must however be
populated by writing to the ``CMP_KUB_DESC`` and ``CMP_TSD_INFO`` registers. It
contains scheduling information for the kernel, so that the kernel function
knows which part of the computation to perform (i.e. the ID of the work-group to
execute).

``CMP_KUB_DESC`` is used to specify the address and size of the Kernel Uniform
Block in device memory, which can store data that is the same for all
invocations of the kernel entry point. This is the address passed to the
``args`` kernel argument.

``CMP_TSD_INFO`` will be described more in detail in the next section. However,
for the purpose of this section it specifies the offset into the KUB where the
work-group scheduling information (``wg_info_t``) is stored.

Before we can set these registers we need to populate the work-group scheduling
info structure, allocate device memory for the structure and copy it to the
device. The ``populateSchedulingInfo`` function does the first part:

.. code:: c++

    // refsi_hal.h
    #include "device/device_if.h" // Added
    ...
    class refsi_hal_device : public hal::hal_device_t {
      ...
     private:
      ...
      bool populateSchedulingInfo(wg_info_t &wg, const hal::hal_ndrange_t &range,
                                  uint32_t work_dim); // Added
      ...
    };

.. code:: c++

    // refsi_hal.cpp
    bool refsi_hal_device::populateSchedulingInfo(wg_info_t &wg,
                                                  const hal::hal_ndrange_t &range,
                                                  uint32_t work_dim) {
      wg.num_dim = work_dim;
      for (int i = 0; i < DIMS; i++) {
        wg.local_size[i] = range.local[i];
        wg.num_groups[i] = (range.global[i] / wg.local_size[i]);
        if ((wg.num_groups[i] * wg.local_size[i]) != range.global[i]) {
          return false;
        }
        wg.global_offset[i] = range.offset[i];
      }
      return true;
    }

The other two parts are performed by the ``allocateKUB`` function, which also
prepares the values to write to the KUB and TSD registers:

.. code:: c++

    // refsi_hal.h
    class refsi_hal_device : public hal::hal_device_t {
      ...
     private:
      ...
      hal::hal_addr_t allocateKUB(const exec_state_t &exec, hal::hal_size_t &kub_desc,
                                  hal::hal_size_t &tsd_info); // Added
      ...
    };

.. code:: c++

    // refsi_hal.cpp
    hal::hal_addr_t refsi_hal_device::allocateKUB(const exec_state_t &exec,
                                                  hal::hal_size_t &kub_desc,
                                                  hal::hal_size_t &tsd_info) {
      auto alignBuffer = [](std::vector<uint8_t> &buffer, uint64_t align) {
        size_t aligned_size = (buffer.size() + align - 1) / align * align;
        buffer.resize(aligned_size);
      };
      std::vector<uint8_t> kub_data;

      // Pack work-group scheduling info into the KUB.
      uint64_t sched_offset = kub_data.size();
      uint64_t sched_size = sizeof(exec_state_t);
      kub_data.resize(kub_data.size() + sched_size);
      memcpy(&kub_data[sched_offset], &exec, sched_size);
      alignBuffer(kub_data, sizeof(uint64_t));

      // Allocate memory for the Kernel Uniform Block and copy it to device memory.
      const uint64_t kub_align = 256;
      alignBuffer(kub_data, kub_align);
      uint64_t kub_size = kub_data.size();
      hal::hal_addr_t kub_addr = mem_alloc(kub_size, kub_align);
      if (!kub_addr || !mem_write(kub_addr, kub_data.data(), kub_size)) {
        mem_free(kub_addr);
        return hal::hal_nullptr;
      }

      // Prepare the CMP register values.
      kub_desc = (kub_addr & 0xffffffffffff) | ((kub_size / kub_align) << 48ull);
      tsd_info = ((sched_offset & 0xffffffull) << 16ull) |
                 ((sched_size & 0xffffffull) << 40ull);

      return kub_addr;
    }

These two new functions are called prior to the command buffer being generated
in `kernel_exec`. Write commands to the ``CMP_REG_KUB_DESC`` and
``CMP_REG_TSD_INFO`` registers are added to the command buffer and the number
of parallel harts to use is set to the device's default value:

.. code:: c++

    // hal_refsi.cpp
    bool refsi_hal_device::kernel_exec(hal::hal_program_t program,
                                       hal::hal_kernel_t kernel,
                                       const hal::hal_ndrange_t *nd_range,
                                       const hal::hal_arg_t *args,
                                       uint32_t num_args, uint32_t work_dim) {
      ...
      
      // Store work-group scheduling info for the kernel in the KUB.
      exec_state_t exec;
      memset(&exec, 0, sizeof(exec_state_t));
      if (!populateSchedulingInfo(exec.wg, *nd_range, work_dim)) {
        return false;
      }
      hal::hal_size_t kub_desc = 0;
      hal::hal_size_t kargs_info = 0;
      hal::hal_size_t tsd_info = 0;
      hal::hal_addr_t kub_addr = allocateKUB(exec, kub_desc, tsd_info);
      if (!kub_addr) {
        return false;
      }
      
      // Encode the command buffer.
      refsi_command_buffer cb;
      size_t num_instances = exec.wg.num_groups[0]; // Changed
      cb.addWRITE_REG64(CMP_REG_ENTRY_PT_FN, kernel_wrapper->symbol);
      cb.addWRITE_REG64(CMP_REG_RETURN_ADDR, elf->find_symbol("kernel_exit"));
      cb.addWRITE_REG64(CMP_REG_KUB_DESC, kub_desc); // Added
      cb.addWRITE_REG64(CMP_REG_TSD_INFO, tsd_info); // Added
      cb.addRUN_KERNEL_SLICE(/* num_harts */ 0, num_instances, 0); // Changed
      cb.addFINISH();

      // Execute the command buffer.
      ...

      mem_free(kub_addr); // Added
      return true;
    }

Also note that the first parameter to the ``RUN_KERNEL_SLICE`` command
(``num_harts``) has changed from one (limit execution to one hart) to zero (use
all available harts).

At the end of this sub-step, two additional clik tests now pass:

.. code:: console

    Failed tests:
      blur
      concatenate_dma
      matrix_multiply
      ternary_async
      vector_add
      vector_add_async
      vector_add_wfv

    Passed:            4 ( 36.4 %)
    Failed:            7 ( 63.6 %)
    Timeouts:          0 (  0.0 %)

The two new passing tests (``hello`` and ``hello_async``) are the only examples
that run kernels which do not make use of the ``args`` parameter passed to the
kernel entry point function. In the next section we will look at how to ensure
that the ``args`` parameter is correctly initialized before executing kernels.
