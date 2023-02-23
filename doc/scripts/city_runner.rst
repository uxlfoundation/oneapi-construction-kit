City Runner
===========

CityRunner is a command-line tool that can run batches of OpenCL Conformance
Test Suite (CTS) tests. Tests can be executed in different ways (e.g. locally,
remotely, using emulation, etc) through the profile system. Multiple tests are
executed in parallel when possible to leverage multi-core systems and minimize
run time.

There are two use cases for CityRunner: automated testing within a Continuous
Integration system (such as Jenkins) and developer testing.

Continuous Integration
----------------------

In this kind of environment a pre-set list of tests is run automatically at
regular intervals and as such testing should be robust (i.e. not require human
intervention). In particular tests should not be allowed to hang and block other
testing jobs (due to issues like deadlocks). Per-test timeout puts an upper
bound to how long an individual test can run before being aborted, aborted tests
are placed in the ``Timeout`` result category. CityRunner can also integrate
with CI systems like Jenkins by generating JUnit test result files which allows
easy tracking of test regressions over time.

.. seealso::
  See our Jenkins handbook page on CTS testing for more information on how
  CityRunner is used in Jenkins.

Developer Testing
-----------------

Developers can also use CityRunner to ensure that their work does not cause
obvious regressions before committing it to source control. Running every
single test usually takes too long to run multiple times a day. It is possible
to specify patterns that filter the list of tests to run. This allows running
small sets of tests (perhaps even only the test that the developer is trying to
fix).

Basic Operation
###############

At a minimum, CityRunner needs the following to run CTS tests:

* A CSV file containing a list of tests to run (e.g.
  ``opencl_conformance_tests_subtests.csv``). This is specified using the ``-s``
  option. For the actual format of this file, see the `CSV File Format`_
  section.
* A directory that contains CTS executables (e.g. ``conformance_test_basic``)
  and is used as the working directory. This is specified using the ``-b``
  option.

For example, let's assume that the current directory contains another directory,
``CTS-output``, where the test executables are located, and a CSV test list
file. Starting a test batch that includes all tests from the CSV files is done
like this:

.. code:: console

  $ /path/to/run_cities.py \
    -s opencl_conformance_tests_subtests_codeplay_wimpy.csv \
    -b CTS-output

  Running 1552 tests using 8 workers.

  [  0 %] [0:0:1/1552] PASS cl12/allocations/multiple_5_image2d_read
  ...

Selecting a single test is done by simply passing its name on the command-line:

.. code:: console

  $ /path/to/run_cities.py -s \
    opencl_conformance_tests_subtests_codeplay_wimpy.csv \
    -b CTS-output \
    cl12/basic/if

  Running 1 tests using 8 workers.

  [100 %] [0:0:1/1] PASS cl12/basic/if

  Finished in 0:00:01

  Passed:        1 (100.0 %)
  Failed:        0 (  0.0 %)
  Timeouts:      0 (  0.0 %)
  Skipped:       0 (  0.0 %)
  CTS Pass:      1 (100.0 %)

By default no test output is shown if the test passes. This can be changed by
passing the verbose ``-v`` option:

.. code:: console

  $ /path/to/run_cities.py \
    -s opencl_conformance_tests_subtests_codeplay_wimpy.csv \
    -b CTS-output \
    cl12/basic/if \
    -v

  Running 1 tests using 8 workers.

  [100 %] [0:0:1/1] PASS cl12/basic/if
  ******************** cl12/basic/if PASS in 0:00:00.189471 ********************
  /path/to/CTS-output/conformance_test_basic if

  Initializing random seed to 0.

  Requesting Default device based on command line for platform index 0 and device index 0
  Compute Device Name = Intel(R) Core(TM) i7-4770K CPU @ 3.50GHz, Compute Device Vendor = Intel(R) Corporation, Compute Device Version = OpenCL 1.2 (Build 8), CL C Version = OpenCL C 1.2
  Supports single precision denormals: YES
  sizeof( void*) = 8  (host)
  sizeof( void*) = 8  (device)
  if...
  IF test passed
  if passed
  PASSED test.
  ********************************************************************************

  Finished in 0:00:01

  Passed:        1 (100.0 %)
  Failed:        0 (  0.0 %)
  Timeouts:      0 (  0.0 %)
  Skipped:       0 (  0.0 %)
  CTS Pass:      1 (100.0 %)

In the previous examples the default OpenCL implementation (i.e. the one that
can be found from the system path) was used. In order to test a different OpenCL
implementation it is possible to alter the system library search path. If the
OpenCL library we want to test is found in the ``bin`` directory in the current
directory, we can select it using the ``-L`` option:

.. code:: console

  $ /path/to/run_cities.py \
    -s opencl_conformance_tests_subtests_codeplay_wimpy.csv \
    -b CTS-output \
    -L bin \
    cl12/basic/if \
    -v

  Running 1 tests using 8 workers.

  [100 %] [0:0:1/1] PASS cl12/basic/if
  ******************** cl12/basic/if PASS in 0:00:00.027088 ********************
  LD_LIBRARY_PATH='/path/to/bin' /path/to/CTS-output/conformance_test_basic if

  Initializing random seed to 0.
  Requesting Default device based on command line for platform index 0 and device index 0
  Compute Device Name = X86, Compute Device Vendor = Codeplay Software Ltd, Compute Device Version = OpenCL 1.2 ComputeSuite::OCL 0.2.0.0, CL C Version = OpenCL C 1.2 ComputeSuite OCL
  Supports single precision denormals: NO
  sizeof( void*) = 8  (host)
  sizeof( void*) = 8  (device)
  if...
  IF test passed
  if passed
  PASSED test.
  ********************************************************************************

  Finished in 0:00:01

  Passed:        1 (100.0 %)
  Failed:        0 (  0.0 %)
  Timeouts:      0 (  0.0 %)
  Skipped:       0 (  0.0 %)
  CTS Pass:      1 (100.0 %)

Profile System
--------------

It is possible to run tests built for architectures other than the local
machine's (e.g., running tests built for Arm when the local machine is x86).
This can be done either by emulating the test executables on the local machine
(using `QEMU`_) or by executing these executables remotely (using tools such as
``ssh`` or ``adb``). The profile system has been designed to allow running
tests in different ways to allow these use cases.

Android Profile
###############

Profiles are selected by using the ``-p`` option on the command-line. For example,
to execute a test on the Android device connected to the local machine, we would
execute the following command:

.. code:: console

  $ /path/to/run_cities.py \
    -s opencl_conformance_tests_subtests_codeplay_wimpy.csv \
    -p android \
    -b /data/local/tmp \
    cl12/basic/if \
    -v

  Running 1 tests using 1 workers.

  [100 %] [0:0:1/1] PASS cl12/basic/if
  ******************** cl12/basic/if PASS in 0:00:00.431332 ********************
  adb shell "cd /data/local/tmp && export LD_LIBRARY_PATH='/data/local/tmp' && /data/local/tmp/conformance_test_basic if"

  Initializing random seed to 0.
  Requesting Default device based on command line for platform index 0 and device index 0
  Compute Device Name = ARM, Compute Device Vendor = Codeplay Software Ltd, Compute Device Version = OpenCL 1.2 ComputeSuite::OCL 0.2.0.0, CL C Version = OpenCL C 1.2 ComputeSuite OCL
  Supports single precision denormals: NO
  sizeof( void*) = 4  (host)
  sizeof( void*) = 4  (device)
  if...
  IF test passed
  if passed
  PASSED test.
  ********************************************************************************

  Finished in 0:00:01

  Passed:        1 (100.0 %)
  Failed:        0 (  0.0 %)
  Timeouts:      0 (  0.0 %)
  Skipped:       0 (  0.0 %)
  CTS Pass:      1 (100.0 %)

Note how the working directory was changed to ``/data/local/tmp``. This is the
directory where the CTS test executables and OpenCL library were previously
copied to on the Android device. The number of workers (number of concurrent
tests) is one as CityRunner is not able to remotely detect the number of cores
on the device.

The ``--device-serial`` option can be used to select a specific Android device
in the case multiple devices are connected to the same machine. Currently only
one Android device can be used. In the future it may be possible to run tests
concurrently on multiple Android devices.

The ``--adb`` option can be used to select a specific ``adb`` executable in
case it is not located in the system path.

SSH Profile
###########

Most profiles also require passing profile-specific options. For examples, to
use the ``ssh`` profile we need to specify the remote host and login:

.. code:: console

  $ /path/to/run_cities.py \
    -s opencl_conformance_tests_subtests_codeplay_wimpy.csv \
    -p ssh \
    --ssh-host arm-device \
    --ssh-user user \
    -b /path/to/ocl \
    cl12/basic/if \
    -v

  Running 1 tests using 1 workers.

  [100 %] [0:0:1/1] PASS cl12/basic/if
  ******************** cl12/basic/if PASS in 0:00:00.791614 ********************
  /usr/bin/ssh "-p 22" user@arm-device "cd /path/to/ocl && export LD_LIBRARY_PATH='/path/to/ocl' && /path/to/ocl/conformance_test_basic if"

  Initializing random seed to 0.
  Requesting Default device based on command line for platform index 0 and device index 0
  Compute Device Name = ARM, Compute Device Vendor = Codeplay Software Ltd, Compute Device Version = OpenCL 1.2 ComputeSuite::OCL 0.2.0.0, CL C Version = OpenCL C 1.2 ComputeSuite OCL
  Supports single precision denormals: NO
  sizeof( void*) = 4  (host)
  sizeof( void*) = 4  (device)
  if...
  IF test passed
  if passed
  PASSED test.
  ********************************************************************************

  Finished in 0:00:01

  Passed:        1 (100.0 %)
  Failed:        0 (  0.0 %)
  Timeouts:      0 (  0.0 %)
  Skipped:       0 (  0.0 %)
  CTS Pass:      1 (100.0 %)

The ``--ssh`` option can be used to select a specific SSH executable in case it
is not located in the system path or to use a specific client (e.g. PuTTY on
Windows).

The ``--ssh-port`` option allows the change the remote port to use (default:
``22``).

Reboot On Fail
^^^^^^^^^^^^^^

The ``--ssh-reboot-on-fail`` option will send a ``reboot`` command to the
device over ssh. This requires root access to the device and is intended to
reset the device in case external issues affect test reliability.

Support For Multiple Devices
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``--ssh-devices`` option allows users to target multiple devices while using ssh
profile. The option expects a file containing values describing username, host
and port number to be used while connecting to a remote device. The format is
as follows:

.. code::

  username_1 hostname_1:port_1
  username_2 hostname_2:port_2

Please note, that the values stored in ssh-devices file take precedence over
those specified using ``-ssh-user``, ``-ssh-host`` or ``-ssh-port``.

In order to be able to perform a hardware reboot when using multiple boards,
users need to provide ``-ssh-pdu-map`` file, that links pdu devices with hosts.
Expected format of the entries is as follows:

.. code::

  device_hostname_1 pdu_hostname_1:pdu_port_1
  device_hostname_2 pdu_hostname_2:pdu_port_2

QEMU Profile
############

Tests built for another architecture can also be emulated on the local machine
using `QEMU`_. This requires a few profile-specific options:

.. code:: console

  $ /path/to/run_cities.py \
    -s /path/to/opencl_conformance_tests_subtests_codeplay_wimpy.csv \
    -p qemu \
    --qemu '/usr/bin/qemu-arm -L /usr/arm-linux-gnueabihf' \
    -b /path/to/build/bin \
    cl12/basic/if \
    -v
  Running 1 tests using 8 workers.

  [100 %] [0:0:0:1/1] PASS cl12/basic/if
  ******************** cl12/basic/if PASS in 0:00:01.405060 ********************
  /usr/bin/qemu-arm -L /usr/arm-linux-gnueabihf /path/to/build/bin/conformance_test_basic if

  Initializing random seed to 0.
  Requesting Default device based on command line for platform index 0 and device index 0
  Compute Device Name = ComputeAorta Arm, Compute Device Vendor = Codeplay Software Ltd., Compute Device Version = OpenCL 1.2 ComputeAorta 1.36.0 LLVM 9.0.0svn (RelWithDebInfo), CL C Version = OpenCL C 1.2 Clang 9.0.0svn
  Supports single precision denormals: NO
  sizeof( void*) = 4  (host)
  sizeof( void*) = 4  (device)
  if...
  IF test passed
  if passed
  PASSED test.

  ********************************************************************************

  Finished in 0:00:01

  Passed:            1 (100.0 %)
  Failed:            0 (  0.0 %)
  Timeouts:          0 (  0.0 %)
  Skipped:           0 (  0.0 %)
  Overall Pass:      1 (100.0 %)
  Overall Fail:      0 (  0.0 %)


Here the ``--qemu`` option is used to select the `QEMU`_ executable that is
able to emulate the executable for the desired architecture, it includes
``-L /usr/arm-linux-gnueabihf`` to specify the ``ld`` prefix path. For example,
``qemu-mipsel`` could have been use to emulate MIPS little endian executables.

Alternatively the ``--ld`` option can be used if an executable is unable to
find a shared library it depends upon. This is incompatible with specifying
``-L /usr/arm-linux-gnueabihf`` as part of the ``--qemu`` option and will
result in a ``loader cannot load itself`` error message.

.. code:: console

  $ /path/to/run_cities.py \
    -s /path/to/opencl_conformance_tests_subtests_codeplay_wimpy.csv \
    -p qemu \
    --qemu /usr/bin/qemu-arm \
    --ld /usr/arm-linux-gnueabihf/lib/ld-linux-armhf.so.3 \
    -L /usr/arm-linux-gnueabihf/lib \
    -L /path/to/build/lib \
    -b /path/to/build/bin \
    cl12/basic/if \
    -v
  Running 1 tests using 8 workers.

  [100 %] [0:0:0:1/1] PASS cl12/basic/if
  ******************** cl12/basic/if PASS in 0:00:01.395500 ********************
  /usr/bin/qemu-arm /usr/arm-linux-gnueabihf/lib/ld-linux-armhf.so.3 --library-path /usr/arm-linux-gnueabihf/lib:/path/to/build/lib /path/to/build/bin/conformance_test_basic if

  Initializing random seed to 0.
  Requesting Default device based on command line for platform index 0 and device index 0
  Compute Device Name = ComputeAorta Arm, Compute Device Vendor = Codeplay Software Ltd., Compute Device Version = OpenCL 1.2 ComputeAorta 1.36.0 LLVM 9.0.0svn (RelWithDebInfo), CL C Version = OpenCL C 1.2 Clang 9.0.0svn
  Supports single precision denormals: NO
  sizeof( void*) = 4  (host)
  sizeof( void*) = 4  (device)
  if...
  IF test passed
  if passed
  PASSED test.

  ********************************************************************************

  Finished in 0:00:01

  Passed:            1 (100.0 %)
  Failed:            0 (  0.0 %)
  Timeouts:          0 (  0.0 %)
  Skipped:           0 (  0.0 %)
  Overall Pass:      1 (100.0 %)
  Overall Fail:      0 (  0.0 %)

.. _QEMU:
  https://www.qemu.org/

Google Test Profile
###################

CityRunner can be used to run tests from `Google Test`_ executables using the
``gtest`` profile. This includes :doc:`/source/cl/test/unitcl`, ``UnitMux``,
``UnitCompiler``, and ``UnitCargo``. Through the `Google Test`_
``--gtest_list_tests`` argument we can query the binary for the list of tests to
run, then invoke them individually with ``--gtest_test_filter``.

As a result if the ``-s`` argument for a CSV file isn't provided to
``run_cities.py`` we can still find tests. A CSV file can still be used with
the ``gtest`` profile but the format is different from CTS tests and takes a
single qualified test name.

Using the ``gtest`` profile provides benefits over standalone execution of the
binary including parallelism, configurable timeouts, and better crash handling.

In order to forward options to the `Google Test`_ executable the
``--gtest_argument`` option can be passed to CityRunner with a quoted string.
Use this multiples times to pass more than one option.

The GTest profile can also be run over SSH, all the CityRunner arguments
available to the SSH profile are inherited by GTest. To enable this feature
combine the ``-p gtest`` and ``--ssh-user`` options.

Any argument passed to ``--gtest_argument`` that contains the special string
``${TEST_NAME}`` will be expanded by CityRunner to the name of the test
executing, when it executes. For example,
``--gtest_argument"--gtest_output=xml:${TEST_NAME}_output.xml"`` will produce
an output file ``expanded_test_name_output.xml`` for each test executed by
CityRunner in the test list.

.. code:: console

  $ /path/to/run_cities.py -b \
    bin/UnitCL \
    -p gtest \
    --gtest_argument="--unitcl_test_include=test/UnitCL/test_include" \
    --gtest_argument="--unitcl_kernel_directory=test/UnitCL/kernels" \
    --gtest_argument="--gtest_filter=Execution*" \
    -v \
    -t 00:10

  ...

  [100 %] [0:1:0:335/336] PASS Execution/ExecutionWG.Compiler_Barrier_03_Odd_Work_Group_Size/31
  ******************** Execution/ExecutionWG.Compiler_Barrier_03_Odd_Work_Group_Size/31 PASS in 0:00:00.130328 ********************
  bin/UnitCL --unitcl_test_include=test/UnitCL/test_include --unitcl_kernel_directory=test/UnitCL/kernels --gtest_filter=Execution* --gtest_filter=Execution/ExecutionWG.Compiler_Barrier_03_Odd_Work_Group_Size/31

  Note: Google Test filter = Execution/ExecutionWG.Compiler_Barrier_03_Odd_Work_Group_Size/31
  [==========] Running 1 test from 1 test case.
  [----------] Global test environment set-up.
  [----------] 1 test from Execution/ExecutionWG
  [ RUN      ] Execution/ExecutionWG.Compiler_Barrier_03_Odd_Work_Group_Size/31
  [       OK ] Execution/ExecutionWG.Compiler_Barrier_03_Odd_Work_Group_Size/31 (33 ms)
  [----------] 1 test from Execution/ExecutionWG (33 ms total)

  [----------] Global test environment tear-down
  [==========] 1 test from 1 test case ran. (37 ms total)
  [  PASSED  ] 1 test.

  ********************************************************************************

  Finished in 0:00:11

  Passed:          335 ( 99.7 %)
  Failed:            0 (  0.0 %)
  Timeouts:          0 (  0.0 %)
  Skipped:           1 (  0.3 %)
  Overall Pass:    335 (100.0 %)
  Overall Fail:      0 (  0.0 %)

.. _Google Test:
  https://github.com/google/googletest
.. _UnitCL:

Response Files
--------------

While the flexibility of the profile system allows a set of tests to be easily
run on devices with different architectures, the downside is long command-lines
due to the many options to fill in. Since many of these options are set to the
same value no matter which tests are run, it would make sense to allow sets of
options to be saved to a file and re-used. The 'response file' feature allows a
list of options to be read from a file and treated as if the options were passed
on the command-line as usual.

Response files are simple text files, where each line represents a command-line
argument. For example, the following ``arm_qemu`` file would contain the options
used to run ARM tests using `QEMU`_::

  -s
  /opencl_conformance_tests_subtests_codeplay_wimpy.csv
  -p
  qemu
  --qemu
  /usr/bin/qemu-arm
  --ld
  /usr/arm-linux-gnueabihf/lib/ld-linux-armhf.so.3
  -L
  /usr/arm-linux-gnueabihf/lib
  -L
  bin
  -b
  CTS-output

Running the ``if`` test would then be done in the following way:

.. code:: console

  $ /path/to/run_cities.py @arm_qemu cl12/basic/if -v


Note how the ``@`` sign precedes the name of the response file to replace by
its content on the command-line. Multiple response files can be loaded in the
same way. This can be useful for running a list of failed tests. Let's suppose
we run all the tests first, generating the ``arm-qemu.fail`` fail file:

.. code:: console

  $ /path/to/run_cities.py @arm_qemu -r arm-qemu.fail
  ...
  Failed tests:
    cl12/spir/atomics
    cl12/spir/basic
    cl12/spir/conversions
    cl12/spir/geometrics
    cl12/spir/half
    cl12/spir/math_brute_force
  ...

Looking at the output of the failed tests revealed the issue with the tests,
which we fix. Now, we can easily re-run just those tests using the response file
feature:

.. code:: console

  $ /path/to/run_cities.py @arm_qemu @arm-qemu.fail
  ...
  [100 %] [0:0:6/6] PASS cl12/spir/math_brute_force
  ...


Other Options
-------------

``--fail-file``, ``-f``
  This option can be used to specify the path to a fail file. A list of failed
  tests is written to this file after tests have been run.

``--jobs``, ``-j``
  This option determines the number of concurrent tests to run. When profiles
  that execute tests on the local machine are selected, this is set to the
  number of cores of the machine by default. For other profiles the default is
  one.

``--repeat``, ``-r``
  This option specifies the number of times to repeat the tests.

``--junit-result-file``, ``-r``
  This option can be used to specify the path to a JUnit XML file. Per-test
  results (``PASS``/``FAIL``/``TIMEOUT``, duration, output message) are written
  to this file using a format that is accepted by Jenkins.

``--log-file`` ``-l``
  This option can be used to specify the path to a log file. The output of
  every test is written to this file as well as the test result summary.

``--color={auto,always,never}``
  This option controls the use of color since not all terminals support this
  feature. When ``auto`` is specified, if ``stdout`` is a tty on a supported
  platform color output will be enabled. When ``always`` is specified color
  output is always emitted. When ``never`` is specified color output is
  completely disabled.

``--timeout``, ``-t``
  This option can be used to limit how long an individual test can run before
  being aborted. By default no timeout is used, tests are never aborted. The
  format is: ``[HH:]MM:SS``

``--relaxed``
  This option can be used to affect CityRunner's exit code. By default exit
  code ``1`` is returned when tests fail. By using this option an exit code
  ``0`` is returned when tests fail.

``--add-lib-path``, ``-L``
  This option can be used to add a directory to the library search path.

``--add-env-var``, ``-e``
  This option can be used to define an environment variable when invoking test
  executables.

``--disabled-source``, ``-d``
  A csv file containing a list of tests to mark as ``Disabled`` in the original
  test list. Tests marked as ``Disabled`` are not run and counted as failures.
  This is only supported for the CTS profile at the moment.

``--ignored-source``, ``-i``
  A csv file containing a list of tests to mark as ``Ignored`` in the original
  test list. Tests marked as ``Ignored`` are not run and not counted.
  This is only supported for the CTS and GTest profiles at the moment.

CSV File Format
---------------

CityRunner uses the CSV file format implicitly defined by the Khronos CTS
runner:

* Each line describes one test and contains two to five fields separated by
  commas:

  * An optional device type specifier that starts with ``CL_DEVICE_TYPE_``.
    CityRunner currently doesn't use this field.
  * A description of the test as human readable information to help describe
    its purpose and group related tests. Dropped by the framework in parsing,
    see ``TestList.from_file()`` in ``test_info.py``.
  * The name of a test executable, followed by a list of arguments to pass to
    the test executable.
  * An optional :ref:`attribute<attributes>` from the following list:
    ``Ignore``, ``Disabled``, ``Unimplemented``.
  * An optional :ref:`pool<pools>` from the following list: ``Normal``,
    ``Thread``, ``Memory``.
* Lines starting with ``#`` are considered to be comments and are ignored.
  Empty lines are also ignored.

.. _attributes:

Attributes
##########

Ignore
  Tests marked with the ``Ignore`` attribute are not run but are counted as a
  pass towards the CTS pass rate.

Disabled
  Tests marked with the ``Disabled`` attribute are expected fails. They are not
  run but included as a ``FAIL`` in the final results.

Unimplemented
  Tests marked with the ``Unimplemented`` attribute are not run but are counted
  as a passed test towards the final CTS pass rate. This attribute is
  associated with tests which have been left unimplemented by the CTS, or test
  things that aren't in CL 1.x.

.. _pools:

Pools
#####

Pools provide more granular control than the ``-j`` flag over how tests are run
in parallel. By restricting concurrency between the most resource intensive
tests we can avoid fails due to not having all the platform's resources
dedicated to their execution.

Normal
  Default if no pool is specified in the CSV, these tests will be
  allowed to run concurrently with any other test.

Thread
  Thread intensive tests which should be constrained in executing
  concurrently with each other. A maximum of half the number of jobs
  specified by ``-j`` will be be available to the thread pool. Default for
  ``conversions`` and ``bruteforce`` CTS tests.

Memory
  Tests which allocate large amounts of memory. Memory pool annotated
  tests can run in parallel with at most one other memory pool assigned test.
  Default for ``allocations`` and ``integer_ops`` CTS tests.

For example, CityRunner at ``-j8`` will be able to run ``8`` tests in parallel.
Of these ``8`` tests, at most two may be memory-intensive jobs and at most four
may be thread-intensive. Tests from both these categories are permitted to run
concurrently with each other, leading to the situation where the ``8``
concurrently running tests are: two memory-intensive, four thread-intensive,
and two other uncategorized normal tests.

Extensions
----------

The extensions folder of CityRunner is designed to contain customer specific
functionality. Python scripts will need to be placed here manually for imports
to pick them up. This segregation prevents sensitive IP and implementation
details leaking into ComputeAorta.
