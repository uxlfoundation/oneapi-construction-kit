Implementing HAL Operations
===========================

This section describes the work needed to implement a new HAL, which consists
mainly of overriding functions in the ``refsi_hal_device`` class and adding an
implementation for each 'HAL operation' defined by the HAL in ``hal_device_t``.
We will go through all of the operations which are required to run a kernel and
periodically run different clik tests to show how implementation progress is
reflected in test output.


.. toctree::
   :maxdepth: 2

   hal-operations-step-01-device-init-cleanup.rst
   hal-operations-step-02-device-memory.rst
   hal-operations-step-03-loading-programs.rst
   hal-operations-step-04-running-kernels.rst
