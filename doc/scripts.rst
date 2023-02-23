Scripts
=======

The ``<root>/scripts`` directory of ComptuteAorta contains utility scripts
to aid with the building and testing of the project. All the scripts contain
source documentation on their purpose and usage.

Scripts are primarily written in Python, and in some cases depend on our
Python ``build_tools`` repository. Files written in other scripting languages,
such as Bash and Windows Batch, are also present for platform specific
functionality.

.. note::
  Both versions 2.7 and 3 of the Python interpreter are currently supported as
  an execution environment.

Jenkins Scripts
---------------

Scripts in the ``<root>/scripts/jenkins`` directory are intended to
automate our testing, so that there is a unified experience between developers
and Jenkins continuous integration.

.. seealso::
  Consult the Jenkins section of the ComputeAorta Handbook for details on how
  these scripts are used in our continuous integration setup.

Sanitizers
##########

ComputeAorta supports building with `sanitizers`_ enabled, where the sanitizer
settings can be configured at both compile-time and run-time. We provide the
following files to allow our CMake build and test targets to define consistent
settings.

tsan_suppressions.txt
  A path to this file is set in the ``TSAN_OPTIONS="suppressions=..."``
  environment variable in the CMake :command:`add_ca_check` command exposed
  by our :doc:`/cmake/AddCA`.

ubsan_blacklist.txt
  Set as the ``-fsanitize-blacklist`` compile option in the
  :doc:`/cmake/Sanitizers`.

.. _sanitizers:
  https://github.com/google/sanitizers/wiki

CTS Summary
###########

The ``cts_summary`` directory contains CSV files designed to work with
:doc:`/scripts/city_runner` by conforming to our testing
:ref:`scripts/city_runner:CSV File Format`.

Frameworks
----------

Larger multi-file projects requiring their own documentation.

.. toctree::
   :maxdepth: 2

   scripts/coverage
   scripts/city_runner
   scripts/fuzz
   scripts/perf
