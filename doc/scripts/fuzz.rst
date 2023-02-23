Fuzzy Testing
=============

It is possible to automate the testing of the OCL compiler and runtime by
utilizing fuzzy testing tools:

1. `CLSmith`_ for random test generation
2. `C-Reduce`_ for reducing the random test
3. `Oclgrind`_ for keeping the reductions that `C-Reduce`_ produces valid

In addition to these tools, at least two other OpenCL implementations are needed
to work as references.

.. _CLSmith:
  https://github.com/ChrisLidbury/CLSmith
.. _OCLgrind:
  https://github.com/jrprice/Oclgrind
.. _C-Reduce:
  https://embed.cs.utah.edu/creduce

Oclgrind
--------

`Oclgrind`_ is a SPIR interpreter and virtual OpenCL device simulator. In
addition to running OpenCL applications it can also check for runtime issues,
such as data races or uninitialized memory accesses. We utilize those tests for
ensuring that the random test cases generated are actually valid OpenCL code.

CLSmith
-------

`CLSmith`_ is a tool for generating random OpenCL kernels to be used for fuzzy
testing compilers and runtime systems. It comes with a helper application that
will initialize the required memory, compile and run the kernel, and finally
print the results returned from the kernel.

The kernels store a hash value of all their variables as their result. This
value is deterministic and thus should remain constant between runs.

`CLSmith`_ is fairly simple to use, and comes with a helper setup script
(``scripts/cl_setup_test.py``). The script will create a directory and copy
there all the required files. Then, `CLSmith`_ can be invoked without any
arguments to produce a random test file ``CLProg.c``. It is also possible to
provide arguments, such as the seed for the random engine and what features to
include (e.g. vectors).

C-Reduce
--------

`C-Reduce`_ is a test case reduction tool for C, C++, and OpenCL programs. It
is inspired by `delta`_ but it takes the idea a step further by also performing
language aware reductions using ``clang`` and ``clex``.

The critical part of `C-Reduce`_ is it's `Interestingness Tests`_. This is a
script (which is not to take any input) that returns ``0`` if the current test
is interesting (that is, it's a valid test that breaks the compiler or the
runtime) or ``1`` otherwise. Choosing the interestingness test correctly is
important, as otherwise `C-Reduce`_ may reduce the test case beyond usefulness.

In order to create a good usefulness test, we first start by running the
generated kernel through one or more reference implementations of OpenCL. By
reference implementation we do not mean an actual reference implementation but
actual implementations provided by other vendors.

If the results from those runs match, then we proceed to run the kernel through
the Codeplay OpenCL implementation. If the result matches the reference
result(s), then no error has occurred and the test case is not interesting.
Currently, we are only checking the actual result produced by the kernel, but
it is possible to extern the test to check for compiler warnings etc.

Finally, if we see a discrepancy between the reference results and the Codeplay
ones, then we proceed to run the test kernel through `Oclgrind`_. This is done
to make sure that the reduced test case is not incorrect OpenCL code, such as
code that utilizes values read from uninitialized variables. As of the time
this is being written, the actual results from `Oclgrind`_ are ignored.

.. _delta:
  http://delta.stage.tigris.org

Preprocessing
-------------

In order to run the tests in `C-Reduce`_ easily (and currently to remove
barrier calls), the interesting tests need to be preprocessed before being sent
to `C-Reduce`_. Currently that entails running them through the ``clang``
preprocessor (to include all the headers etc) and removing all the barrier
calls.

Files
-----

interesting.py
  The `Interestingness Tests`_, as described above

find_and_reduce.py
  A simple script that runs `CLSmith`_, preprocesses, and then reduces
  interesting test cases automatically.

Getting Things Running
----------------------

There are several prerequisites for the scripts to work.

``interesting.py`` requires `Oclgrind`_ and the ``cl_launcher`` application
from `CLSmith`_. There are command line switches for both of them, in case they
are not in your ``PATH``. Also, it requires the `PyOpenCL`_ python library for
getting the available OpenCL platforms and their names.

.. seealso::
 See the builtin ``interesting.py --help`` for more details

``find_and_reduce.py`` additionally requires `Clang`_ (tested with v3.8) and
`CLSmith`_.

.. _Clang:
  http://clang.llvm.org
.. _PyOpenCL:
  https://pypi.org/project/pyopencl/

Installing the prerequisites
############################

* `CLSmith`_
* `C-Reduce`_
* `Oclgrind`_
* `LLVM`_
* `Clang`_
* `libclc`_

The following instructions are intended as a quick start guide on building all
the requirements. As the projects might change, please check their respective
documentation if you run into any issues.

.. _LLVM:
  http://llvm.org
.. _libclc:
  http://libclc.llvm.org

Install CLSmith
^^^^^^^^^^^^^^^

`CLSmith`_ uses the CMake build system. In order to build, do the following:

.. code:: console

  $ mkdir build
  $ cd build
  $ cmake ..
  $ cmake --build . --config Release


Install C-Reduce
^^^^^^^^^^^^^^^^

`C-Reduce`_ is a `Perl`_ application, and requires the packages listed
`here <https://github.com/csmith-project/creduce/blob/master/INSTALL.md#prereqs>`_

.. note::
  The `Perl`_ dependencies can be found on Ubuntu 14.04 in the packages
  ``libexporter-lite-perl`` etc.

.. _Perl:
  https://www.perl.org
.. _Flex:
  https://github.com/westes/flex

LLVM and Clang
^^^^^^^^^^^^^^

It is possible to use the system provided `LLVM`_ and `Clang`_, assuming that it
is at the correct version. It is recommended to use a custom built version to
avoid any issues.

.. code:: console

  $ mkdir build
  $ cd build
  $ cmake -DLLVM_EXTERNAL_CLANG_SOURCE_DIR=/path/to/clang/sources \
          -DCMAKE_BUILD_TYPE=Release ../
  $ cmake --build . --target install --config Release


Please note that the above command will install LLVM in the system directories
(``/usr/`` or ``/usr/local`` by default in Linux) and it might override any
preexisting system installations. It might be better to install LLVM on a
different directory and then adjust the environment accordingly.

Install Oclgrind
^^^^^^^^^^^^^^^^

.. code:: console

  $ mkdir build
  $ cmake -DCMAKE_BUILD_TYPE=Release ../
  $ cmake --build . --target install --config Release


As with LLVM, this will install `Oclgrind`_ in the system directories, which
might have unwanted side effects.

Putting everything together
###########################

After setting up all the prerequisites, then the easiest way to get things
running is by using the `CLSmith`_ ``cl_setup_test.py`` script. It takes as an
argument a directory name and then proceeds to create that directory and copy
all the necessary files (including the ``CLSmith`` executable) there. All you
have to do then is navigate into this directory and run the
``find_and_reduce.py`` script. By default the script expects to find all the
required executables in your ``PATH``, except from ``cl_launcher`` and
``CLSmith``, which expects them to be in the current working directory. So make
sure that you either do or that you pass the correct command line options. It
is better to provide those options as full paths, since the helper scripts
might be executed from a different working directory.

Interestingness Tests
---------------------

There are two interestingness tests. It is possible to run combinations of them,
in which case **all** tests need to be interesting for the testcase to be
considered as interesting.

1. Reference run: This test will run the OpenCL kernel against any OpenCL
   implementations found in the system and it will compare the ComputeAorta one
   with the rest of them. If there is a result mismatch then the test case is
   considered interesting. This is the default test but it can be disabled with
   the ``--no-references`` option.

2. Vectorization: This test, enabled with the ``--check-if-vectorized`` option,
   checks if the vectorizer was able to vectorize the given kernel. You
   probably want to also give the ``--vecz`` option as well, or the vectorizer
   will never run and the cases will always be uninteresting.
