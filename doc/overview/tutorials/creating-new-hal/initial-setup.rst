Initial Setup
-------------

Checking Out the Source
^^^^^^^^^^^^^^^^^^^^^^^

The following elements of the oneAPI Construction Kit will need to be used as part
of the creation of the RefSi HAL:
 
* clik: contains the clik runtime libraries as well as a small test suite. This
  is under the top level of the oneAPI Construction Kit.
* hal_refsi_tutorial: contains the skeleton for the RefSi HAL we will create in
  this tutorial. This is under ``examples/hals/hal_refsi_tutorial``.
* refsidrv: contains a driver that controls a virtual RefSi device. This is under
  ``modules/mux/external/refsidrv``.
* hal: contains headers needed to interface with a HAL. At top level of the oneAPI
  Construction Kit.
* riscv-isa-sim: contains the Spike RISC-V simulator, used to simulate the
  RISC-V accelerator cores contained in the RefSi device. Stored under
  ``refsidrv/external``.

Some directories will need to be copied from the ``oneapi-construction-kit`` :
``hal_refsi_tutorial`` and ``refsidrv``. The remainder of this tutorial will
assume that the environment variable ``OCK`` has been set to the base of the
``oneAPI Construction Kit``. Setup and the copy can be done as follows:

.. code:: console

    $ mkdir refsi_tutorial_part1
    $ cd refsi_tutorial_part1
    $ export OCK=<path_to_oneapi-construction-kit>
    $ cp $OCK/examples/hal/hal_refsi_tutorial .
    $ cp $OCK/modules/mux/external/refsidrv hal_refsi_tutorial/external

The resulting source code layout from running the above commands is the following:

.. code::

    refsi_tutorial_part1
        hal_refsi_tutorial/ -> hal_refsi_tutorial repository, 'tutorial1_start' branch
            external/
                refsidrv/ -> refsidrv repository, 'tutorial1' branch
                    external/
                        riscv-isa-sim/ -> submodule of refsidrv

Installing a RISC-V toolchain
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A RISC-V toolchain or GNU RISC-V toolchain is required to build the RefSi HAL.
When using Ubuntu 20.04 the toolchain can be simply installed using the Ubuntu
package manager without additional CMake setup.

For other Linux systems, you'll need to set the CMake variable,
``RISCV_TOOLCHAIN_DIR`` for clang and ``RISCV_GNU_TOOLCHAIN_DIR`` for GCC
pointing to it's build directory. However, if ``RISCV_GNU_TOOLCHAIN_DIR`` is not
set it will be default to ``/usr/``.

Since the oneAPI Construction Kit is most often built against a LLVM and Clang toolchain,
it may be convenient to use the same toolchain to build the parts of the HAL
that need to be cross-compiled. In order to do so, simply set ``RISCV_TOOLCHAIN_DIR``
to the value of the ``CA_LLVM_INSTALL_DIR`` CMake variable.

A RISC-V triple toolchain or GNU RISC-V triple toolchain can be specified to
build the RefSi HAL. The ``RISCV_TOOLCHAIN_TRIPLE`` is set to ``riscv64`` by
default, and the ``RISCV_GNU_TOOLCHAIN_TRIPLE`` is set to ``riscv64-linux-gnu``
by default. However, a different target architecture can be used by manually
configuring the ``RISCV_TOOLCHAIN_TRIPLE`` CMake variable for clang or
``RISCV_GNU_TOOLCHAIN_DIR`` for GCC.

See 'Other Linux system' for an example.

Ubuntu 20.04
~~~~~~~~~~~~

The following packages need to be installed:

.. code:: console

    $ sudo apt install gcc-9-riscv64-linux-gnu g++-9-riscv64-linux-gnu

Other Linux system
~~~~~~~~~~~~~~~~~~

The RV64 toolchain can be built from source. First, to retrieve the source code:

.. code:: console

    $ cd refsi_tutorial_part1
    $ git clone https://github.com/riscv/riscv-gnu-toolchain
    $ cd riscv-gnu-toolchain
    $ git submodule update --init --recursive

Once the source code has been retrieved, a RV64 toolchain can be built with the
following commands:

.. code:: console

    $ sudo mkdir -p /opt/riscv64
    $ sudo chown $(whoami):$(whoami) /opt/riscv64
    $ ./configure --prefix=/opt/riscv64 --with-arch=rv64gc
    $ make linux

The following list of options need to be passed to CMake in the next section:

.. code::

    -DRISCV_GNU_TOOLCHAIN_DIR=/opt/riscv64 -DRISCV_GNU_TOOLCHAIN_TRIPLE=riscv64-unknown-linux-gnu


Building clik and the Skeleton RefSi HAL
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once all of the relevant repositories have been checked out in the source tree
as above, and a RISC-V has been installed on the system, the next step is to
build clik and the skeleton RefSi HAL to ensure that the source tree has been
set up correctly. This can be done with the following commands:

.. code:: console

    $ cd path/to/refsi_tutorial_part1
    $ mkdir build
    $ cd build
    $ cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCLIK_HAL_NAME=refsi_tutorial -DCLIK_EXTERNAL_HAL_DIR=$PWD/../hal_refsi_tutorial $OCK/clik
      ...
      -- Found HAL: cpu
      -- Found HAL: refsi_tutorial
      -- Configuring done
      -- Generating done
      -- Build files have been written to: path/to/refsi_tutorial_part1/build
    $ ninja hal_refsi_tutorial clik_runtime_sync clik_runtime_async
      ...
      [305/305] Linking CXX shared library lib/libhal_refsi_tutorial.so

The ``cmake`` command above requires several options to be set in order to build the
RefSi HAL alongside clik and to ensure clik examples target the appropriate work
scheduling mode:

* Setting ``CMAKE_BUILD_TYPE`` to ``Debug`` instructs CMake to build libraries and
  executables in debug mode. This is not required but improves the debugging
  experience.
* Setting ``CLIK_HAL_NAME`` to ``refsi_tutorial`` lets clik know the name of the
  device HAL library to load when creating clik devices.
* Setting ``CLIK_EXTERNAL_HAL_DIR`` to the absolute path of the ``hal_refsi_tutorial``
  directory lets clik know where to look for the source of the HAL we are going
  to develop in this tutorial.

As mentioned in the preceding section, on some Linux systems additional CMake
variables (name starting with ``RISCV_TOOLCHAIN_``) are needed to configure the
location and triple for the RISC-V toolchain.
