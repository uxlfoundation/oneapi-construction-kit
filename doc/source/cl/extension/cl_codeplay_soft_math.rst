Soft Math - ``cl_codeplay_soft_math``
=====================================

Name String
-----------

``cl_codeplay_soft_math``

Contact
-------

* Ewan Crawford, Codeplay Software Ltd. (ewan 'at' codeplay.com)

Contributors
------------

* Ewan Crawford, Codeplay Software Ltd.
* Neil Henning, Codeplay Software Ltd.

Version
-------

Version 1, September 12, 2016

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

The goal of this extension is to allow developers to control when hardware
accelerated math functions are enabled. Being able to force a software path is
useful for debugging accuracy issues within kernels, as well as verifying
software maths library code which would otherwise not be covered in testing.

Although hardware implementations of all OpenCL builtins will be disabled, a
particular use case for this feature regards the native floating point maths
builtins. These are intended to be run on hardware, and so have undefined
precision requirements to avoid prohibiting the goal of speed over accuracy.
Software implementations of native builtins may use algorithmic optimizations
instead to take advantage of the lax precision requirements, and having a
mechanism to test the suitability of the software path for a particular
application can be useful.

Modifications to the OpenCL API Specification
---------------------------------------------

Append to the end of Section 5.6.4.3 "Optimization Options"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``-cl-codeplay-soft-math``
   Set that hardware accelerated math optimizations should be disabled, and
   instead a software math path should be used.

Revision History
----------------

+-----+------------+---------------+--------------------------+
| Rev | Data       | Author        | Changes                  |
+=====+============+===============+==========================+
| 1   | 2016/09/12 | Ewan Crawford | Initial proposal.        |
+-----+------------+---------------+--------------------------+
| 2   | 2020/05/13 | Ewan Crawford | Expand overview wording. |
+-----+------------+---------------+--------------------------+
