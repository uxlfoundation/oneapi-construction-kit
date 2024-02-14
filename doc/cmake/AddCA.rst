AddCA Module
============

A tenet of modern CMake is to avoid setting compiler flags manually, as they
won't work across all compilers and platforms unless guarded. Unfortunately due
to the extensive range of toolchains which oneAPI Construction Kit supports
utilizing bespoke compiler settings is unavoidable. We try to isolate all these
platform specific configurations in the module ``cmake/AddCA.cmake`` using
`generator expressions`_ to create variables for compiler options and
definitions, set as :cmake:variable:`CA_COMPILE_OPTIONS` and
:cmake:variable:`CA_COMPILE_DEFINITIONS` respectively.

.. seealso::
  For guidance on when and why we use `generator expressions`_ see the
  :ref:`cmake:Generator Expression Usage` section.

``AddCA.cmake`` then consumes these variables in the macros it exposes to wrap
common CMake functions, such as :cmake:command:`add_ca_library`,
:cmake:command:`add_ca_executable`, and :cmake:command:`add_ca_subdirectory`.
These wrappers allow us to add the project wide compiler settings at a central
location rather than scattered over the codebase, and to set target
dependencies on any other targets which could be conditionally required to
support our various
:ref:`CA build options <developer-guide:oneAPI Construction Kit Cmake Options>`
for debugging and instrumentation.

The :cmake-command:`target_compile_options` and
:cmake-command:`target_compile_definitions` CMake commands are the preferred
way of adding flags in CMake, leading to the cleanest code by transitively
passing on flags to dependent targets. Alternative
:cmake-command:`add_compile_options` is based on directory properties, and
`CMAKE_CXX_FLAGS`_ is global and doesn't work with `generator expressions`_.

Our :cmake:variable:`CA_COMPILE_OPTIONS` and
:cmake:variable:`CA_COMPILE_DEFINITIONS` variables are added to
:cmake-command:`target_compile_options` and
:cmake-command:`target_compile_definitions` using a ``PRIVATE`` scope, rather
than ``PUBLIC`` or ``INTERFACE``, meaning that the options won't be propagated
to any other targets which depend on the target we are modifing in the wrapper.
This behavior is preferred over propagating the properties to the new target,
where flags like ``-Werror`` could get set on linked application code which
isn't warning clean.

.. _generator expressions:
 https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html
.. _CMAKE_CXX_FLAGS:
 https://cmake.org/cmake/help/latest/envvar/CXXFLAGS.html

Argument Parsing
################

CMake provides the useful command :cmake-command:`cmake_parse_arguments`, which
facilitates complex argument parsing behaviors and is used to implement many of
our ``add_ca`` prefixed helper functions in the ``AddCA.cmake`` module. We make
use of it to parse single and multiple value keyword arguments, utilizing the
first of the two available :cmake-command:`cmake_parse_arguments` function
signatures. This returns the parsed values as variables beginning with the
parameterized ``prefix``.

.. code-block:: cmake

  # Signature AddCA.cmake helper functions invoke
  cmake_parse_arguments(<prefix> <options> <one_value_keywords>
                        <multi_value_keywords> <args>...)


A case study of this is our helper function :cmake:command:`add_ca_check`,
generating ``check-ock`` and ``check-ock-<name>`` build targets used by
continuous integration to verify a baseline of correctness.
:cmake:command:`add_ca_check` takes a single positional argument ``name``, for
the target to generate a test for. A new testing target called
``check-ock-${name}`` is created from the ``name`` argument and a dependency on
``check-ock-${name}`` is added to the ``check`` target.

Additional :cmake:command:`add_ca_check` options to configure the testing
target are parsed by forwarding on ``$ARGN`` `arguments`_ to
:cmake-command:`cmake_parse_arguments` via the ``<args>`` parameter, such as
multi-value keyword ``COMMAND`` for defining the command to run for test
invocation. ``NOENUMLATE`` and ``NOGLOBAL`` are also declared as flags via the
``<options>`` parameter, parsed as either ``TRUE`` or ``FALSE``.

.. code-block:: cmake

  # Parse add_ca_check $ARGN arguments
  cmake_parse_arguments(args
    "NOEMULATE;NOGLOBAL" "" "CLEAN;COMMAND;DEPENDS;ENVIRONMENT" ${ARGN})

After parsing is complete the results will available as variables prefixed with
'``args_``', e.g ``$args_COMMAND``, for use in subsequent
:cmake:command:`add_ca_check` logic.

.. _arguments:
 https://cmake.org/cmake/help/latest/command/function.html#arguments

Commands and Variables
######################

.. cmake-module:: ../../cmake/AddCA.cmake
