Coverage Scripts
================

Documentation for coverage scripts in ``<root>/scripts/coverage``.

Build Tools & Packages
######################

The coverage Python script needs several tools which should be installed.
Build standard versions should be used with any subsequent deviations for
coverage purposes noted in the current document. The tools are:

* `Python`_ The python scripting language.
* `CMake`_ A cross-platform build system generator.
* `Gcovr`_ A Gcov extension generating human-readable coverage data in a
  variety of output formats. Version **4.1** was used during development.
  We have since upgraded to **4.2**.
* `junit-xml`_ A python package to generate JUnit XML test result outputs.

.. _Python:
  https://www.python.org
.. _CMake:
  https://cmake.org
.. _Gcovr:
  https://www.gcovr.com
.. _junit-xml:
  https://github.com/kyrus/python-junit-xml.git

Building Source & Coverage Exclusions
#####################################

* All source for which coverage information is to be generated should be
  compiled using build standard versions of the following tools, with any
  subsequent deviations for coverage purposes noted in this document:

  * `gcc <https://gcc.gnu.org>`_
  * `clang <http://clang/llvm.org>`_
* All source should be compiled using the ``--coverage`` compiler and linker
  option to enable raw coverage data generation. This results in the creation
  of ``file.gcno`` files at compile-time, and ``file.gcda`` files at run-time.
  Coverage tooling (`Gcovr`_) analyses the content of these files and produces
  human-readable (and consolidated) reports. See the
  `gcc man page page <https://linux.die.net/man/1/gcc>`_ for further details on
  the coverage option. (Note that it is also accepted by the local ``build.py``
  build script.)
* Generally the less optimization used, the more accurate the code coverage
  results will be, so - while the following options are not mandatory - they
  are recommended:

  * Compile with no inlining:
    ``-fno-inline -fno-inline-small-functions -fno-default-inline``

  * Compile with no optimization: ``-O0``
* `Gcovr`_ 'exclusion markers' can be added to the source code within comments.
  Any source so marked is ignored by coverage tooling. Current markers are:

  GCOVR_EXCL_LINE
    Lines containing this marker will be excluded.

  GCOVR_EXCL_START
    Begins an excluded section. Current line excluded.

  GCOVR_EXCL_STOP
    Ends an excluded section. Current line not excluded.

Running Coverage: coverage.py
#############################

``coverage.py`` is the main coverage tester wrapper script.

Coverage "base" directory
-------------------------

All coverage scripting and tooling has the notion of a 'base build directory'.
By default this is the same directory from which the source is initially
built i.e. ``<workspace>/build``. ``coverage.py`` should be run from there.

Command-line Usage
------------------

For low-level information on ``coverage.py`` options and features use the
``--help`` option.

.. code:: console

  $ coverage.py --help

And also see the full ``coverage.py`` options list below.

To perform a coverage test suite run, specify:

#. The path(s) to the test suite(s) that the compiled source should be run
   against.
#. The options that each test suite requires.
#. The module(s) (i.e. source and associated object directory 'pairs') to which
   the coverage should apply.
#. Any coverage script options.

   .. code:: console

     $ coverage.py [--COVERAGE_OPTIONS]* [--test-suite TEST_SUITE_PATH \
                   [TEST_SUITE_OPTIONS]*]+ [--module SOURCE_DIRECTORY  \
                   OBJECT_DIRECTORY]+

XML Input File Option
---------------------

To run coverage on many test suites and modules in a single run the
``--xml-input`` option can be used. It takes a path to an XML file, which has
the format:

.. code:: XML

  <?xml version="1.0"?>
  <coverage>
    <property name="coverage_option_name1" value="coverage_option_value1"/>
    <property name="coverage_option_name2" value="coverage_option_value2"/>
    <testsuite path="path/to/test/exe1" flags="--some option --another --one"/>
    <testsuite path="path/to/test/exe2" flags="--some option --another --one"/>
    <module sources="path/to/module/sources1" objects="path/to/module/objects1"/>
    <module sources="path/to/module/sources2" objects="path/to/module/objects2"/>
  </coverage>

Test suites, modules and properties (aka ``coverage.py`` script options) can be
added according to taste.  Property names are the same as long-format
``coverage.py`` option strings without the ``--`` prefix. Where an
option/property is specified both on the command line and in the ``.xml`` file,
the ``.xml`` file setting is used.

Jenkins coverage jobs use build-time-generated XML Input Files which are
retained as artifacts of each build.

Coverage Filesets and Gcovr Workflow
####################################

Compiling with ``--coverage`` tells the compiler to generate a ``.gcno`` file
for each source file. These files, located in the same directory as the
corresponding  ``.o`` files, contain coverage information and symbols.

During each test suite run, ``.gcda`` files will also be generated, again in
the object files directory. These files contain counters and location
information about functions, branches, and each line of the source. Note that
``.gcda`` files are cumulative, so if they are not purged (e.g. before a second
execution of the same test suite) new coverage information will be added.

`Gcovr`_ output files (plain text, XML or HTML) are generated from ``.gcno``
and ``.gcda`` files and include a consolidated summary of coverage results.
(`Gcovr`_ calls Gcov internally when producing these output files and will
delete any  intermediate ``.gcov`` files by default.) There is no fixed suffix
for the `Gcovr`_ output files.

To generate Cobertura-format XML, `Gcovr`_ is invoked by the following command
line within the ``coverage.py`` wrapper script for each module directory pair
and test suite in turn:

.. code:: sh

  $ gcovr -o GCOVR_OUTPUT_FILE -f MODULE_SOURCE_DIRECTORY --xml_pretty \
          MODULE_OBJECT_DIRECTORY

`Gcovr`_ must already exist on the users' ``$PATH`` when called from
``coverage.py``.

`Gcovr`_ has the notion of a 'base build directory' and is typically run
(via ``coverage.py``) from ``<workspace>/build``. All source file paths listed
in the coverage data are referenced from there.

Following each run of ``coverage.py``, the following classes of files are also
created in the 'build' directory:

``coverage*``
  All retained artifacts and (non-intermediate, non-empty) output files
  from the coverage process have this filename prefix.

``coverage_*.gcovr.xml``
  All Gcovr ``.xml`` coverage output files use this filename format. These
  files are passed to the reporting tool (`Cobertura`_).

``*.xml``
  Any ``.xml`` file without the ``coverage*`` prefix is a JUnit output file
  from the test suite runs.

.. _Cobertura:
  https://plugins.jenkins.io/cobertura/

coverage.py Options
######################

Additional options available for the ``coverage.py`` script:

``--junit-xml-output``
  Generates an XML file in JUnit format summarising failed coverage tests.
  Filename format:  ``[TEST_SUITE_NAME]-coverage-[TIMESTAMP].xml``

``--lcov-html-output``
  Generates HTML based coverage output for each test suite. Folder format:
  ``[TEST_SUITE_NAME]-coverage-html-[TIMESTAMP]``

``--csv-output``
  Generates a CSV file summarizing coverage results of your test suites.
  Filename format: ``[TEST_SUITE_NAME]-coverage-[TIMESTAMP].csv``

``--cover-filter``
  Followed by one or more regexps. Functions and branches which match those
  regexps will be ignored, and tagged as warnings.

``--check-regression``
  Check code coverage regression by comparing run results and presenting the
  output graphically as a ``.png`` file.

``--no-branches``
  Disable branches' checking.

``--no-functions``
  Disable functions' checking.

``--no-module-reporting``
  Disable reporting about modules on ``stdout``.

``--no-test-suite-reporting``
  Disable reporting about test suites on ``stdout``.

``--exclude-modules | -e``
  Specify source paths of modules you want to ignore.

``--only-functions | -f``
  Launch this script only to check not called functions. Not executed and not
  tested branches won't be checked.

``--no-intermediate-files | -i``
  JUnit XML and LCOV HTML intermediate outputs are not generated. Only total
  results files are generated.

``--output-directory | -o``
  Specify an existing directory to store temporary files and coverage files
  (JUnit, Lcov, CSV).

``--percentage | -p``
  Change the limit from which you can consider a test's set as valid, and so,
  displayed in green.

``--quiet | -q``
  Display nothing on ``stdout`` during script's execution.

``--recursive | -r``
  If specified modules contain sub-modules, those will be tested. If this
  option is not enabled, sub-modules will be ignored.

``--threads | -j``
  Specify the number of threads to use to for this script. By default, every
  available CPU threads will be involved.

``--verbose | -v``
  Provide more information on ``stdout``, but not in other outputs.

coverage.py Error Values
########################

Error values returned by the ``coverage.py`` script and their meaning:

``0``
  Success.

``1``
  Returned in case of bad OS, this script works only on Linux.

``2``
  A problem occurred during option flags parsing.

``3``
  A test suite executable passed as parameter doesn't exist.

``4``
  A module directory (source or object) passed as parameter doesn't  exist.

``5``
  The output directory passed as parameter doesn't exist.

``6``
  A problem occurred invoking a process.

``7``
  The percentage passed as argument with ``--percentage`` is invalid.
  This percentage has to be less than or equal to 100 and greater than or equal
  to 75.

``8``
  The format of the file passed to ``--check-regression`` is invalid.

``9``
  The number of threads passed to ``--threads`` is invalid.

Coverage CMake Integration
##########################

.. seealso::
  For more in-depth documentation of coverage specific to CMake see
  :doc:`/cmake/Coverage`.

A ``coverage.cmake`` file is provided in the source code ``coverage`` folder.
This file can be added to the project build system by use of the ``include``
function. The only ``FIXME`` in this file is to specify the path to the code
coverage script.  Projects should also be built using ``-DENABLE_COVERAGE=ON``

There are four functions in the CMake file:

``add_coverage_modules(sources, objects ...)``
  Allows 'source directory/object directory' pairs to be added to the list of
  modules to target.

``add_coverage_test_suite(test_suite, flags)``
  This function adds a test suite and flags to the list of test suites to be
  run by the script.

``edit_coverage_xml_input()``
  This generates the XML Input File which can be passed to the script in the
  file specified by ``COVERAGE_XML_INPUT``.

``add_coverage_custom_target()``
  Create a custom target named ``coverage``. This target will start the
  coverage script, with the previously generated XML file as an argument.

After adding these functions, the following command line can be used to start
code coverage analysis.

.. code:: sh

  $ make coverage

License
#######

Copyright (C) Codeplay Software Limited. All Rights Reserved.
