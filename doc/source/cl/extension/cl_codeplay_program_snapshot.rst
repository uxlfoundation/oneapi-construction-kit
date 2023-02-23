Program Snapshot - ``cl_codeplay_program_snapshot``
===================================================

Name String
-----------

``cl_codeplay_program_snapshot``

Contact
-------

* Ewan Crawford, Codeplay Software Ltd. (ewan 'at' codeplay.com)

Contributors
------------

* Ewan Crawford, Codeplay Software Ltd.
* Bjoern Knafla, Codeplay Software Ltd.

Version
-------

Version 2, February 21, 2019

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

This extension provides a snapshot mechanism allowing user code to capture
program objects at different stages of compilation.

New Tokens
----------

Accepted as *format* parameter to `clRequestProgramSnapshotCODEPLAY`_:

+-------------------------------------------+-------+
| Enumeration                               | Value |
+===========================================+=======+
| CL_PROGRAM_BINARY_FORMAT_DEFAULT_CODEPLAY | 0x0   |
+-------------------------------------------+-------+
| CL_PROGRAM_BINARY_FORMAT_BINARY_CODEPLAY  | 0x1   |
+-------------------------------------------+-------+
| CL_PROGRAM_BINARY_FORMAT_TEXT_CODEPLAY    | 0x2   |
+-------------------------------------------+-------+

New Types
---------

Parameter type of `clRequestProgramSnapshotCODEPLAY`_, defining the signature
of the callback functor.

.. code-block:: c

   typedef void (*cl_codeplay_snapshot_callback_t)(size_t snapshot_size,
                                                   const char* snapshot_data,
                                                   void* callback_data,
                                                   void* user_data)

Parameter type of *format* to `clRequestProgramSnapshotCODEPLAY`_:

.. code-block:: c

   typedef cl_uint cl_codeplay_program_binary_format;

New API Functions
-----------------

.. code-block:: c

   cl_int clRequestProgramSnapshotListCODEPLAY(cl_program program,
                                               cl_device_id device,
                                               const char **stages,
                                               cl_uint *num_stages)

.. code-block:: c

   cl_int clRequestProgramSnapshotCODEPLAY(cl_program program,
                                           cl_device_id device,
                                           const char *stage,
                                           cl_codeplay_program_binary_format format,
                                           cl_codeplay_snapshot_callback_t callback,
                                           void *user_data)

Modifications to the OpenCL API Specification
---------------------------------------------

Add a new Section 5.XX - "Snapshots":
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The snapshot mechanism is the primary method of debugging compiler
transformations. This is done by dumping the program object (taking a snapshot)
at a specific stage of compilation in order to inspect the effects of compiler
passes. So allowing the user to more effectively debug and tune their code. In
this section, we discuss how snapshots are created and returned using OpenCL
runtime API functions.

Add a new Section 5.XX.1 - "Listing Snapshots":
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _clRequestProgramSnapshotListCODEPLAY:

The function

.. code-block:: c

   cl_int clRequestProgramSnapshotListCODEPLAY(cl_program program,
                                               cl_device_id device,
                                               const char **stages,
                                               cl_uint *num_stages)

is used to query a device's compiler for a list of available snapshot
stages. The list returned is ordered according to their order in the
compilation pipeline.

*program*
   is the program object. It must not be NULL.

*device*
   is a device associated with the context of program. It must not be NULL.

*stages*
   is an array of C strings to be populated with snapshot stage names. The
   number of snapshot stage names returned is the minimum of the value
   specified by num_stages or the number of available snapshot stages. If
   stages is NULL, this argument is ignored and the number of available
   snapshot stages can be queried with the num_stages argument.

*num_stages*
   is the number of snapshot stage names entries that can be added to stages.
   If stages is not NULL, then num_stages must be greater than zero. If stages
   is NULL, num_stages returns the number of snapshot stage names available.

`clRequestProgramSnapshotListCODEPLAY`_ returns ``CL_SUCCESS`` if the function
is executed successfully. Otherwise, it returns one of the following errors:

* ``CL_INVALID_PROGRAM`` if <program> is not a valid program.
* ``CL_INVALID_DEVICE`` if <device> is not a valid device or not in the list of
  devices associated with program's context.
* ``CL_INVALID_ARG_VALUE`` if both <stages> and <num_stages> are NULL.
* ``CL_INVALID_VALUE`` if snapshot stages could not be queried.

Add a new Section 5.XX.2 - "Setting snapshots":
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _clRequestProgramSnapshotCODEPLAY:

The function

.. code-block:: c

   cl_int clRequestProgramSnapshotCODEPLAY(cl_program program,
                                           cl_device_id device,
                                           const char *stage,
                                           cl_codeplay_program_binary_format format,
                                           cl_codeplay_snapshot_callback_t callback,
                                           void *user data)

sets a callback to be invoked by the runtime when a specified snapshot stage
has been completed during compilation. This snapshot can be recorded by means
of the provided user callback. Note that clCreateProgramFromBinary may accept
binary snapshots, but this is not guaranteed.

At most a single snapshot stage can be selected at any given moment in time.

*program*
   is the program object. It must not be NULL.

*device*
   is a device associated with the context of program. It must not be NULL.

*stage*
   is the name of the snapshot stage to use. It must match a valid stage name
   previously returned by `clRequestProgramSnapshotListCODEPLAY`_.

*format*
   is an enumeration constant that identifies the format of the snapshot stage
   dump of the program object to pass into the callback. The format of the
   snapshot can be set to text or binary but will ultimately depend on the
   stage. As text could mean assembly or IR. It can be one of the following
   values:

   * ``CL_PROGRAM_BINARY_FORMAT_DEFAULT_CODEPLAY`` to capture snapshot in
     default format.
   * ``CL_PROGRAM_BINARY_FORMAT_BINARY_CODEPLAY`` to capture snapshot in text
     format.
   * ``CL_PROGRAM_BINARY_FORMAT_TEXT_CODEPLAY`` to capture snapshot in binary
     format.

*callback*
   is the user's snapshot callback function to invoke when the selected
   snapshot stage is completed.

   This callback function might be called
   asynchronously by the snapshot extension. It is the user's responsibility to
   ensure that the callback function is thread-safe. The parameters to this
   callback function are:

   * *snapshot_size* is the size in bytes of the snapshot data.
   * *snapshot_data* is the snapshot byte data in the selected format. The
     pointer is only valid during callback execution.
   * *callback_data* is an opaque pointer to internal runtime information. It
     is only valid during callback execution.
   * *user_data* is the user provided data. May be NULL.

*user_data*
   is the user provided data to pass to the snapshot callback on invocation,
   may be NULL.

`clRequestProgramSnapshotCODEPLAY`_ returns ``CL_SUCCESS`` if the function is
executed successfully. Otherwise, it returns one of the following errors:

* ``CL_INVALID_PROGRAM`` if *program* is not a valid program.
* ``CL_INVALID_DEVICE`` if *device* is not a valid device or not in the list of
  devices associated with program's context.
* ``CL_INVALID_ARG_VALUE`` if *callback* is NULL, or *stage* is not a valid
  snapshot stage, or format is not a valid value.
* ``CL_INVALID_PROGRAM_EXECUTABLE`` if compilation has already occurred.
* ``CL_INVALID_VALUE`` if snapshot stages can not be queried.

Revision History
----------------

+-----+------------+---------------+-------------------+
| Rev | Data       | Author        | Changes           |
+=====+============+===============+===================+
| 1   | 2016/05/14 | Ewan Crawford | Initial proposal. |
+-----+------------+---------------+-------------------+
| 2   | 2019/03/19 | Bjoern Knafla | Refine wording.   |
+-----+------------+---------------+-------------------+
