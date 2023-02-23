Extra Build Options - ``cl_codeplay_extra_build_options``
=========================================================

Name String
-----------

``cl_codeplay_extra_build_options``

Contact
-------

*  Ewan Crawford, Codeplay Software Ltd. (ewan 'at' codeplay.com)

Contributors
------------

*  Ewan Crawford, Codeplay Software Ltd.
*  Guillaume Marques, Codeplay Software Ltd.
*  Neil Henning, Codeplay Software Ltd.
*  Amy Worthington, Codeplay Software Ltd.
*  Stefano Cherubin, Codeplay Software Ltd.
*  Aaron Greig, Codeplay Software Ltd.
*  Kenneth Benzie, Codeplay Software Ltd.

Version
-------

Version 12, February 8, 2023

Number
------

OpenCL Extension #XX

Status
------

Proposal

Dependencies
------------

OpenCL 1.2 is required.

Overview
--------

The extra build options extension allows developers to provide additional build
options for different purposes.

Modifications to the OpenCL API Specification
---------------------------------------------

Add a new Section 5.6.4.7 - "Options for Debugging"

``-cl-llvm-stats``
   Print stats from optimization passes.

``-cl-vec={none|loop|slp|all}``
   Enables Loop and/or SLP Vectorization passes prior to VECZ vectorization.

``-cl-precache-local-sizes=<sizes>``
   Specifies a list of local work group sizes which should be used to pre-cache
   results of the stages of kernel compilation that require a local size.
   These stages normally take place when an ND range is enqueued, but by
   completing them early and storing the results for later lookup (pre-caching)
   them we can remove all compilation overhead from the enqueue function. A
   local work group size is expressed in the form of up to three comma separated
   numbers, multiple sizes can be given in a colon separated list. The same
   restrictions apply to these sizes as apply to sizes passed to
   `clEnqueueNDRangeKernel`_, see the spec for that entry point for info on
   those constraints.

Revision History
----------------

+-----+------------+-------------------+----------------------------------+
| Rev | Date       | Author            | Changes                          |
+=====+============+===================+==================================+
| 1   | 2016/03/11 | Ewan Crawford     | -g build flag                    |
+-----+------------+-------------------+----------------------------------+
| 2   | 2016/06/25 | Ewan Crawford     | -S build option                  |
+-----+------------+-------------------+----------------------------------+
| 3   | 2018/03/27 | Guillaume Marques | -cl-wfv and -cl-llvm-stats flags |
+-----+------------+-------------------+----------------------------------+
| 4   | 2018/03/27 | Neil Henning      | -cl-dma                          |
+-----+------------+-------------------+----------------------------------+
| 5   | 2019/05/13 | Guillaume Marques | -cl-wfv auto option              |
+-----+------------+-------------------+----------------------------------+
| 6   | 2019/07/21 | Amy Worthington   | -cl-vec                          |
+-----+------------+-------------------+----------------------------------+
| 7   | 2020/06/17 | Stefano Cherubin  | -cl-wi-order                     |
+-----+------------+-------------------+----------------------------------+
| 8   | 2020/10/23 | Aaron Greig       | -cl-precache-local-sizes         |
+-----+------------+-------------------+----------------------------------+
| 9   | 2021/07/23 | Ori Sky Farrell   | Remove -cl-wfv option            |
+-----+------------+-------------------+----------------------------------+
| 10  | 2021/11/12 | Kenneth Benzie    | Remove -g and -S                 |
+-----+------------+-------------------+----------------------------------+
| 11  | 2022/10/25 | Colin Davidson    | Remove -cl-dma                   |
+-----+------------+-------------------+----------------------------------+
| 12  | 2020/02/08 | Amy Worthington   | Remove -cl-wi-order              |
+-----+------------+-------------------+----------------------------------+

.. _clEnqueueNDRangeKernel:
   https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clEnqueueNDRangeKernel
