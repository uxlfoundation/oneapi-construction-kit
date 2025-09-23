spirv-ll
========

The ``spirv-ll`` module provides a static library that implements translation
from binary SPIR-V modules to an ``llvm::Module`` and a command line tool which
outputs LLVM bitcode ``.ll`` files created from binary SPIR-V modules
implemented using the static library.

Standalone Mode
---------------

The ``spirv-ll`` module can be built as part of the oneAPI Construction Kit
CMake project or as a standalone CMake project. To build ``spirv-ll`` standalone
use the following command from the ``modules/spirv-ll`` directory, where
``$LLVMInstall`` is the path to your LLVM build install directory:
::

   cmake . -Bbuild -DCMAKE_BUILD_TYPE=Release -DCA_LLVM_INSTALL_DIR=$LLVMInstall

Internal SPIR-V Extensions
--------------------------

SPIR-V extensions

.. toctree::
   :maxdepth: 1

   spirv-ll/extension/spv_codeplay_group_async_copies

External SPIR-V extensions
--------------------------

* `SPV_INTEL_kernel_attributes
  <https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_kernel_attributes.html>`_
  Note that only ``MaxWorkDimINTEL`` mode is currently processed.
