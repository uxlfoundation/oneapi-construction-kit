Welcome to oneAPI Construction Kit's documentation!
###################################################

The documentation herein refers to version |release| of the oneAPI Construction Kit.

The heterogeneous language toolkit.
===================================

The oneAPI Construction Kit is a framework to provide implementations of open standards, such as OpenCL,
for a wide range of devices. The oneAPI Construction Kit can be used to build with the oneAPI Toolkit. The oneAPI Toolkit
includes support for various open standards, such as OpenMP, SYCL, and DPC++. DPC++ is based on the SYCL programming
model, which allows to write single-source C++ code that can target both CPUs and GPUs. To get more information on oneAPI,
please visit `oneAPI <https://www.intel.com/content/www/us/en/developer/tools/oneapi/overview.html>`_.

.. note::
 It is not intended to be used as a standalone OpenCL implementation. It does not support the oneAPI Level Zero API.

.. toctree::
    :maxdepth: 3

    overview
    getting-started
    specifications
    developer-guide
    tutorials
    design
    source/cl
    modules
    api-reference
    cmake
    scripts
