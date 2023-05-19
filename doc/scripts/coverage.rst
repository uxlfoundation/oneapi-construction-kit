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

                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

    TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

    1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

    2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

    3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

    4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

    5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

    6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

    7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

    8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

    9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

    END OF TERMS AND CONDITIONS

    APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "[]"
      replaced with your own identifying information. (Don't include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

    Copyright [yyyy] [name of copyright owner]

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


---- LLVM Exceptions to the Apache 2.0 License ----

As an exception, if, as a result of your compiling your source code, portions
of this Software are embedded into an Object form of such source code, you
may redistribute such embedded portions in such Object form without complying
with the conditions of Sections 4(a), 4(b) and 4(d) of the License.

In addition, if you combine or link compiled forms of this Software with
software that is licensed under the GPLv2 ("Combined Software") and if a
court of competent jurisdiction determines that the patent provision (Section
3), the indemnity provision (Section 9) or other Section of the License
conflicts with the conditions of the GPLv2, you may retroactively and
prospectively choose to deem waived or otherwise exclude such Section(s) of
the License, but only in their entirety and only with respect to the Combined
Software.
