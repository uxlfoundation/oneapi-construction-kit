Kernel Debug - ``cl_codeplay_kernel_debug``
===========================================

Name String
-----------

``cl_codeplay_kernel_debug``

Contact
-------

*  Ewan Crawford, Codeplay Software Ltd. (ewan 'at' codeplay.com)

Contributors
------------

*  Ewan Crawford, Codeplay Software Ltd.
*  Kenneth Benzie, Codeplay Software Ltd.

Version
-------

Version 3, November 12, 2021

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

The kernel debug extension grants developers the ability to specify build
options which enable attaching a debugger to a kernel being executed on a
device.

This is done by allowing cl_program objects to be built with debug information.
Secondly letting developers set the source file directly means that the
debugger can display source code even when the program source is generated at
runtime.

Modifications to the OpenCL API Specification
---------------------------------------------

Add a new Section 5.6.4.7 - "Options for Debugging"

``-g``
   Build program with debug info.

``-S <path/to/source/file>``
   Point debug information to a source file on disk. If this does not exist,
   the runtime creates the file with cached source.

Revision History
----------------

+-----+------------+-------------------+---------------------------------------+
| Rev | Date       | Author            | Changes                               |
+=====+============+===================+=======================================+
| 1   | 2016/03/11 | Ewan Crawford     | -g build flag                         |
+-----+------------+-------------------+---------------------------------------+
| 2   | 2016/06/25 | Ewan Crawford     | -S build option                       |
+-----+------------+-------------------+---------------------------------------+
| 3   | 2021/11/12 | Kenneth Benzie    | Add -g and -S, forked from            |
|     |            |                   | ``cl_codeplay_extra_build_options``   |
+-----+------------+-------------------+---------------------------------------+
