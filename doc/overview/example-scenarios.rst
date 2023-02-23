Example Hardware Feature Scenarios
==================================

This section describes how to expose hardware-specific features to the user
through OpenCL and SYCL when implementing a ComputeMux target for a given
hardware platform. Most of these features are not expected to be core parts of
the two standards mentioned above and would likely be exposed as extensions,
however existing functionality can also be targeted. Due to the general-purpose
nature of these standards, this section has performance-related features in mind
when describing how to expose the hardware's capabilities to the user.

.. toctree::
   :maxdepth: 2

   example-scenarios/mapping-custom-instructions-to-builtin-functions
   example-scenarios/mapping-custom-ip-blocks-to-builtin-kernels
   example-scenarios/how-to-support-large-scratchpad-memories
   example-scenarios/mapping-algorithms-to-vector-hardware
   example-scenarios/refsi-in-kernel-dma
