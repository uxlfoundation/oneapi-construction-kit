Running clik Tests
------------------

At this stage, clik tests (examples) have been successfully built, including the
kernels, which have been cross-compiled to the RISC-V architecture. However,
attempting to run an example will fail:

.. code:: console

    $ bin/copy_buffer
      Unable to create a clik device.

This is because the skeleton RefSi HAL provided for this tutorial contains stubs
(empty functions) for all of the operations which are required to be implemented
by the HAL, including the operation used to create a new HAL device which is
used by clik. How to implement these operations will be described in the
following sections.

Before starting to implement the HAL, we will briefly explain how to run all
clik tests as a suite and obtain the number of passed and failed tests

.. code:: console

    $ ninja check
      [100 %] [11:0:0/11] FAIL concatenate_dma
      ******************** concatenate_dma FAIL in 0:00:00.006947 ********************
      Unable to create a clik device.

      ********************************************************************************

      Failed tests:
        blur
        concatenate_dma
        copy_buffer
        copy_buffer_async
        hello
        hello_async
        matrix_multiply
        ternary_async
        vector_add
        vector_add_async
        vector_add_wfv

      Passed:            0 (  0.0 %)
      Failed:           11 (100.0 %)
      Timeouts:          0 (  0.0 %)

As can be seen above, all clik tests are currently failing. This is to be
expected when the RefSi HAL is still at the skeleton stage. Nevertheless, it is
useful to run ``ninja check`` periodically while developing the HAL, to confirm
that a particular operation has been implemented correctly or that a new change
to the source code has not caused any regression.

