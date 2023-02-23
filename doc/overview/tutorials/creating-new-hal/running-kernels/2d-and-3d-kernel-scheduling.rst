2D and 3D Kernel Scheduling
===========================

The two remaining clik test failures (``blur`` and ``matrix_multiply``) use 2D
kernels. The command buffer generated in the previous sections contains a single
``CMP_RUN_KERNEL_SLICE`` command with ``wg.num_groups[0]`` as the number of
instances. For 2D and 3D kernels, this means that only work-groups in the first
dimensions are computed, i.e. for 2D kernels only groups from the first 'row'
are executed.

In order to compute all the work-groups in the second and third dimension we can
encode multiple ``CMP_RUN_KERNEL_SLICE`` commands in the command buffer, each
slice command receiving a different slice ID. The number of kernel slice
commands will be equal to the product of the number of groups in the second and
third dimension:

.. code:: c++

    // Encode the command buffer.
    refsi_command_buffer cb;
    cb.addWRITE_REG64(CMP_REG_ENTRY_PT_FN, kernel_wrapper->symbol);
    cb.addWRITE_REG64(CMP_REG_RETURN_ADDR, elf->find_symbol("kernel_exit"));
    cb.addWRITE_REG64(CMP_REG_KUB_DESC, kub_desc);
    cb.addWRITE_REG64(CMP_REG_KARGS_INFO, kargs_info);
    cb.addWRITE_REG64(CMP_REG_TSD_INFO, tsd_info);
    uint32_t max_harts = 0;
    uint64_t num_instances = exec.wg.num_groups[0];
    uint64_t num_slices = 0;
    num_slices = (work_dim == 2) ? exec.wg.num_groups[1] : 1;
    num_slices = (work_dim == 3) ? exec.wg.num_groups[1] * exec.wg.num_groups[2]
                               : num_slices;
    for (uint64_t i = 0; i < num_slices; i++) {
      cb.addRUN_KERNEL_SLICE(/* num_harts */ max_harts, num_instances, i);
    }
    cb.addFINISH();

Running the ``matrix_multiply`` example shows multiple ``CMP_RUN_KERNEL_SLICE``
commands being generated. With this example there are 2 work-groups in the first
dimension and 32 in the second dimension. This results in 32 slice
commands, each of which has 2 instances:

.. code:: console

    $ REFSI_DEBUG=cmp bin/matrix_multiply
    [CMP] Starting.
    [CMP] Starting to execute command buffer at 0xbff0ffb8.
    [CMP] CMP_WRITE_REG64(WINDOW_BASE0, 0x10000)
    [CMP] CMP_WRITE_REG64(WINDOW_TARGET0, 0xbff10000)
    [CMP] CMP_WRITE_REG64(WINDOW_SCALE0, 0x0)
    [CMP] CMP_WRITE_REG64(WINDOW_MODE0, 0xeffff00000001)
    [CMP] CMP_FINISH
    [CMP] Finished executing command buffer in 0.000 s
    Using device 'RefSi M1 Tutorial'
    Running matrix_multiply example (Global size: 32x32, local size: 16x1)
    [CMP] Starting to execute command buffer at 0xbff0cba8.
    [CMP] CMP_WRITE_REG64(ENTRY_PT_FN, 0x10088)
    [CMP] CMP_WRITE_REG64(RETURN_ADDR, 0x10108)
    [CMP] CMP_WRITE_REG64(KUB_DESC, 0x10000bff0cf00)
    [CMP] CMP_WRITE_REG64(KARGS_INFO, 0x0)
    [CMP] CMP_WRITE_REG64(TSD_INFO, 0xc00000280000)
    [CMP] CMP_RUN_KERNEL_SLICE(n=2, slice_id=0, max_harts=0)
    [CMP] CMP_RUN_KERNEL_SLICE(n=2, slice_id=1, max_harts=0)
    [CMP] CMP_RUN_KERNEL_SLICE(n=2, slice_id=2, max_harts=0)
    [CMP] CMP_RUN_KERNEL_SLICE(n=2, slice_id=3, max_harts=0)
    ...
    [CMP] CMP_RUN_KERNEL_SLICE(n=2, slice_id=29, max_harts=0)
    [CMP] CMP_RUN_KERNEL_SLICE(n=2, slice_id=30, max_harts=0)
    [CMP] CMP_RUN_KERNEL_SLICE(n=2, slice_id=31, max_harts=0)
    [CMP] CMP_FINISH
    [CMP] Finished executing command buffer in 0.047 s
    Results validated successfully.
    [CMP] Requesting stop.
    [CMP] Stopping.

For 2D kernels, the slice ID is equal to the group ID in the second dimension.
For 3D kernels, the entry point function needs to split the slice ID into two
group IDs using the `num_groups` information from the scheduling info.

At the end of this sub-step, all clik tests now pass:

.. code:: console

    [100 %] [0:0:11/11] PASS blur

    Passed:           11 (100.0 %)
    Failed:            0 (  0.0 %)
    Timeouts:          0 (  0.0 %)
