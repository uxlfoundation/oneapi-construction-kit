Kernel Argument Packing
=======================

In order to pass kernel arguments to the entry point function, the arguments
must be packed (serialized) into a buffer and included in the Kernel Uniform
Block. Serialization of kernel arguments can be delegated to the
``hal_argpack_t`` argument packer, an utility class included in the HAL
interface. The ``CMP_REG_KARGS_INFO`` register must also be initialized with the
offset into the KUB where the packed arguments are stored.

Since the starting address of the KUB is passed to the kernel entry point as the
``args`` kernel arguments parameter, we have to pack kernel arguments at the
start of the KUB, before the work-group scheduling info. As a side-effect,
``CMP_REG_KARGS_INFO`` can be initialized by simply clearing the register since
the kernel arguments offset into the KUB will always be zero.

This can be done by passing the kernels arguments to the ``allocateKUB``
function and updating the KUB generation code to use the ``hal_argpack_t`` class
for packing arguments in the KUB buffer, before the work-group scheduling info
is packed:

.. code:: c++

    // refsi_hal.cpp
    #include "arg_pack.h"   // Added
    ...
    hal::hal_addr_t refsi_hal_device::allocateKUB(const hal::hal_arg_t *args,
                                                  uint32_t num_args,
                                                  const exec_state_t &exec,
                                                  hal::hal_size_t &kub_desc,
                                                  hal::hal_size_t &tsd_info) {
      ...
      
      std::vector<uint8_t> kub_data;

      // Pack arguments.
      hal::util::hal_argpack_t packer(64);
      if (!packer.build(args, num_args)) {
        return false;
      }
      kub_data.resize(packer.size());
      memcpy(kub_data.data(), packer.data(), packer.size());
      alignBuffer(kub_data, sizeof(uint64_t));

      // Pack work-group scheduling info into the KUB.
      ...
    }

The ``kernel_exec`` operation also needs to be changed to use the updated
``allocateKUB`` function:

.. code:: c++

    hal::hal_size_t kub_desc = 0;
    hal::hal_size_t kargs_info = 0;   // Added
    hal::hal_size_t tsd_info = 0;
    hal::hal_addr_t kub_addr = allocateKUB(args, num_args, exec, kub_desc,
                                           tsd_info); // Updated
    if (!kub_addr) {
      return false;
    }

Finally, a command that writes to the ``CMP_REG_KARGS_INFO`` register needs to
be added:

.. code:: c++

    // Encode the command buffer.
    refsi_command_buffer cb;
    size_t num_instances = exec.wg.num_groups[0];
    cb.addWRITE_REG64(CMP_REG_ENTRY_PT_FN, kernel_wrapper->symbol);
    cb.addWRITE_REG64(CMP_REG_KUB_DESC, kub_desc);
    cb.addWRITE_REG64(CMP_REG_KARGS_INFO, kargs_info);    // Added
    cb.addWRITE_REG64(CMP_REG_TSD_INFO, tsd_info);
    cb.addRUN_KERNEL_SLICE(* num_harts */ 0, num_instances, 0);
    cb.addFINISH();

Please note that the ``kargs_info`` value is set to zero instead of being
computed based on the offset of the packed arguments into the KUB. Since the
arguments are always packed at the beginning of the KUB, the offset is always
zero.

At the end of this sub-step, all clik tests but two now pass:

.. code:: console

    Failed tests:
      blur
      matrix_multiply

    Passed:            9 ( 81.8 %)
    Failed:            2 ( 18.2 %)
    Timeouts:          0 (  0.0 %)

