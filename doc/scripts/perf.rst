Linux Perf Support Script
=========================

This python scripting framework is meant to be used in conjunction with
the enablement of the linux tool `perf`_. To enable `perf`_ support refer to
:ref:`developer-guide:Perf Support for Linux CPU kernels`.

.. _perf:
  https://perf.wiki.kernel.org/index.php/Main_Page

perf-annotate.py
----------------

The ``perf-annotate.py`` script is the main driver that glues all the other
scripts together to annotate OpenCL JITed kernels and display the output to
the screen. It is invoked in the following way:

.. code:: console

  $ perf-annotate.py <optional-arguments> <object-file(s)> <map-file(s)>

.. note::
  Both Python 3 and Python 2 interpreters can be used, although it is advisable
  to use Python 3, as Python 2.7 is now
  `End-of-Life <https://www.python.org/dev/peps/pep-0373/#update>`_.

All arguments are assumed to be CSV lists. Optional arguments can be
specified multiple times. The command line argument parser will combine these
and internally create a single CSV list for that argument. Multiple input
files can be specified for positional parameters as a CSV list.

Positional Arguments
####################

object-file(s)
  A CSV list of all relevant object files that have been dumped by ComputeAorta
  when 'perf events' have been recorded for a particular run.

map-file(s)
  A CSV list of all map files that have been dumped

Optional Arguments
##################

The optional arguments are:

``-p``, ``--prog=\<program-name\>``
 A CSV of program names that the user is interested in searching for within the
 perf-logs. The perf logs will contain samples from all executables that
 triggered a particular event during the perf tool's sampling window. Multiple
 ``-p`` options on the command-line will be merged to make one single CSV option
 list.

``-f``, ``--func=\<function-names\>``
  A CSV of function names generated while JITing. Usually on host ComputeAorta
  will JIT function names of the form ``__mux_host_%u``. Only names found in
  the map-file can be annotated. If this option is not specified, then by
  default, all functions found within perf-logs will be annotated and displayed
  (Beware : potentially large). Multiple ``-f`` options on the command-line
  will be merged to make one single CSV option list.

``-e``, ``--event=\<event-names\>``
  A CSV of event names that the user is interested in parsing. This option is
  useful to filter only the events the user is interested in viewing. Multiple
  ``-e`` options on the command-line will be merged to make one single
  CSV option list. All events that are listed as filters should match exactly
  the same as the options provided when perf record was invoked to record
  samples. Eg. if ``cache-references:u`` was requested while
  recording the samples, mention ``cache-references:u`` and not just
  ``cache-references``.

  Possible events:
    * ``branch-misses``
    * ``branch-instructions``
    * ``cache-references``
    * ``cache-misses``
    * ``cpu-cycles``
    * ``instructions``
    * ``mem-stores``
    * ``mem-load``

  .. seealso::
    The below linux command-line will give an exhaustive list of events.

    .. code:: console

      $ perf list
