Whole Function Vectorization - ``cl_codeplay_wfv``
==================================================

Name Strings
------------

``cl_codeplay_wfv``

Contact
-------

*  Ori Sky Farrell, Codeplay Software Ltd. (ori at codeplay.com)

Contributors
------------

*  Ori Sky Farrell, Codeplay Software Ltd.

Notice
------

Copyright (c) 2021 Codeplay Software Ltd.

Status
------

Proposal

Version
-------

Built On: 2021-06-01
Version: 0.1.0

Dependencies
------------

This extension is written against the OpenCL Specification Version 3.0.6.

This extension requires OpenCL 1.0.

Overview
--------

The whole function vectorization extension provides a mechanism to vectorize an
OpenCL kernel across the primary work-item dimension.

New API Enums
-------------

Accepted as the *param_name* parameter to `clGetKernelWFVInfoCODEPLAY`_:

+-----------------------------------+-------+
| Enumeration                       | Value |
+===================================+=======+
| ``CL_KERNEL_WFV_STATUS_CODEPLAY`` | 0x1   |
+-----------------------------------+-------+
| ``CL_KERNEL_WFV_WIDTHS_CODEPLAY`` | 0x2   |
+-----------------------------------+-------+

.. _cl_kernel_wfv_status_codeplay:

Accepted as the query result of `clGetKernelWFVInfoCODEPLAY`_ for
``CL_KERNEL_WFV_STATUS_CODEPLAY``:

+-----------------------------+-------+
| Enumeration                 | Value |
+=============================+=======+
| ``CL_WFV_SUCCESS_CODEPLAY`` |  0    |
+-----------------------------+-------+
| ``CL_WFV_NONE_CODEPLAY``    | -1    |
+-----------------------------+-------+
| ``CL_WFV_ERROR_CODEPLAY``   | -2    |
+-----------------------------+-------+

New Types
---------

Accepted as the *param_name* parameter to `clGetKernelWFVInfoCODEPLAY`_:

.. code-block:: c

   typedef cl_uint cl_kernel_wfv_info_codeplay;

Returned as the query result of `clGetKernelWFVInfoCODEPLAY`_ for
``CL_KERNEL_WFV_STATUS_CODEPLAY``:

.. code-block:: c

   typedef cl_int cl_kernel_wfv_status_codeplay;

New API Functions
-----------------

.. code-block:: c

   cl_int clGetKernelWFVInfoCODEPLAY(cl_kernel kernel,
                                     cl_device_id device,
                                     cl_uint work_dim,
                                     const size_t *global_work_size,
                                     const size_t *local_work_size,
                                     cl_kernel_wfv_info_codeplay param_name,
                                     size_t param_value_size,
                                     void *param_value,
                                     size_t *param_value_size_ret);

Modifications to the OpenCL API Specification
---------------------------------------------

Add a new Section 5.8.6.X, Options for Whole Function Vectorization:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``-cl-wfv={always|auto|never}``
   Control whether or not a program is compiled with whole function
   vectorization enabled.

   always:
      Always perform whole function vectorization.

   auto:
      Only perform whole function vectorization if determined by the OpenCL
      implementation to be optimal.

   never:
      Never perform whole function vectorization.

Add a new Section 5.9.X, Kernel WFV Queries:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _clGetKernelWFVInfoCODEPLAY:

To return whole function vectorization information about a kernel object, call
the function:

.. code-block:: c

   cl_int clGetKernelWFVInfoCODEPLAY(cl_kernel kernel,
                                     cl_device_id device,
                                     cl_uint work_dim,
                                     const size_t *global_work_size,
                                     const size_t *local_work_size,
                                     cl_kernel_wfv_info_codeplay param_name,
                                     size_t param_value_size,
                                     void *param_value,
                                     size_t *param_value_size_ret);

*kernel*
   Specifies the kernel object being queried.

*device*
   Identifies a specific device in the list of devices associated with *kernel*.
   The list of devices is the list of devices in the OpenCL context that is
   associated with *kernel*. If the list of devices associated with *kernel* is
   a single device, *device* can be a ``NULL`` value.

*work_dim*
   The number of dimensions used to specify the work-items in the work-group.
   *work_dim* must be greater than zero and less than or equal to
   ``CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS``.

*global_work_size*
   Points to an array of *work_dim* unsigned values that describe the number of
   global work-items in *work_dim* dimensions that are intended to execute the
   kernel function. The total number of global work-items is computed as
   *global_work_size* [0] × ... × *global_work_size* [*work_dim* - 1]. If
   *global_work_size* and *local_work_size* are ``NULL``, *global_work_size*
   will not be used by the OpenCL runtime when choosing a work-group size. If
   *global_work_size* is ``NULL`` and *local_work_size* is not ``NULL``,
   *global_work_size* will not be used to determine if the explicitly specified
   *local_work_size* is valid.

*local_work_size*
   Points to an array of *work_dim* unsigned values that describe the number
   of work-items that make up a work-group (also referred to as the size of the
   work-group) that is intended to execute the kernel specified by *kernel*. If
   *local_work_size* is ``NULL``, the OpenCL runtime may choose a work-group
   size. If non-uniform work-groups are supported and *local_work_size* is
   ``NULL``, the OpenCL runtime may choose a uniform or non-uniform work-group
   size. The total number of work-items in the work-group must be less than or
   equal to the ``CL_KERNEL_WORK_GROUP_SIZE`` value specified in the
   `Kernel Object Device Queries`_ table, and the number of work-items specified
   in *local_work_size* [0], ..., *local_work_size* [*work_dim* - 1] must be
   less than or equal to the corresponding values specified by
   ``CL_DEVICE_MAX_WORK_ITEM_SIZES`` [0], ..., ``CL_DEVICE_MAX_WORK_ITEM_SIZES``
   [*work_dim* - 1]. The explicitly specified *local_work_size* will be used to
   determine how to break the global work-items specified by *global_work_size*
   into appropriate work-group instances.

*param_name*
   Specifies the information to query. The list of supported *param_name* types
   and the information returned in *param_value* by
   `clGetKernelWFVInfoCODEPLAY`_ is described in the `Kernel WFV Queries`_
   table.

*param_value*
   A pointer to memory where the appropriate result being queried is returned.
   If *param_value* is ``NULL``, it is ignored.

*param_value_size*
   Used to specify the size in bytes of memory pointed to by *param_value*. This
   size must be ≥ size of return type as described in the
   `Kernel WFV Queries`_ table.

*param_value_size_ret*
   Returns the actual size in bytes of data being queried by *param_name*. If
   *param_value_size_ret* is ``NULL``, it is ignored.

`clGetKernelWFVInfoCODEPLAY`_ returns ``CL_SUCCESS`` if the function is
executed successfully. Otherwise, it returns one of the following errors:

* ``CL_INVALID_DEVICE`` if *device* is not in the list of devices associated
  with *kernel* or if *device* is ``NULL`` but there is more than one device
  associated with *kernel*.
* ``CL_INVALID_KERNEL`` if *kernel* is not a valid kernel object.
* ``CL_INVALID_VALUE`` if *param_name* is not valid, or if size in bytes
  specified by *param_value_size* is < size of return type as described in the
  `Kernel WFV Queries`_ table and *param_value* is not ``NULL``.
* ``CL_INVALID_WORK_DIMENSION`` if *work_dim* is not a valid value (i.e. a value
  between 1 and ``CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS``).
* ``CL_INVALID_GLOBAL_WORK_SIZE`` if any of the values specified in
  *global_work_size* [0], ..., *global_work_size* [*work_dim* - 1] exceed the
  maximum value representable by ``size_t`` on the device on which the
  kernel-instance is intended to be enqueued.
* ``CL_INVALID_GLOBAL_WORK_SIZE`` if any of the values specified in
  *global_work_size* [0], ..., *global_work_size* [*work_dim* - 1] are equal to
  zero when the OpenCL version is less than 2.1.
* ``CL_INVALID_WORK_GROUP_SIZE`` if *local_work_size* is specified and does not
  match the required work-group size for *kernel* in the program source.
* ``CL_INVALID_WORK_GROUP_SIZE`` if *local_work_size* is specified and is not
  consistent with the required number of sub-groups for *kernel* in the program
  source.
* ``CL_INVALID_WORK_GROUP_SIZE`` if *local_work_size* is specified and the total
  number of work-items in the work-group computed as *local_work_size* [0] × ...
  *local_work_size* [*work_dim* - 1] is greater than the value specified by
  ``CL_KERNEL_WORK_GROUP_SIZE`` in the `Kernel Object Device Queries`_ table.
* ``CL_INVALID_WORK_GROUP_SIZE`` if the work-group size must be uniform and the
  *local_work_size* is not `NULL`, or is not equal to the required work-group
  size specified in the kernel source.
* ``CL_INVALID_WORK_GROUP_SIZE`` if the number of work-items specified in any of
  *local_work_size* [0], ... *local_work_size* [*work_dim* - 1] is equal to
  zero.
* ``CL_INVALID_WORK_ITEM_SIZE`` if the number of work-items specified in any of
  *local_work_size* [0], ..., *local_work_size* [*work_dim* - 1] is greater than
  the corresponding values specified by ``CL_DEVICE_MAX_WORK_ITEM_SIZES`` [0],
  ..., ``CL_DEVICE_MAX_WORK_ITEM_SIZES`` [*work_dim* - 1].
* ``CL_OUT_OF_RESOURCES`` if there is a failure to allocate resources required
  by the OpenCL implementation on the device.
* ``CL_OUT_OF_HOST_MEMORY`` if there is a failure to allocate resources required
  by the OpenCL implementation on the host.

Add a new Table 3X, `clGetKernelWFVInfoCODEPLAY`_ parameter queries:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _Kernel WFV Queries:

+-----------------------------------+----------------------------------+-----------------------------------------------+
| Kernel Info                       | Return Type                      | Description                                   |
+===================================+==================================+===============================================+
| ``CL_KERNEL_WFV_STATUS_CODEPLAY`` | `cl_kernel_wfv_status_codeplay`_ | Returns the status of whole function          |
|                                   |                                  | vectorization for *kernel*.                   |
|                                   |                                  |                                               |
|                                   |                                  | This can be one of the following:             |
|                                   |                                  |                                               |
|                                   |                                  | ``CL_WFV_NONE_CODEPLAY``. The status returned |
|                                   |                                  | if whole function vectorization has not been  |
|                                   |                                  | performed on the specified kernel object for  |
|                                   |                                  | the specified device. This status will always |
|                                   |                                  | be returned if the underlying program was     |
|                                   |                                  | created with `clCreateProgramWithBinary`_.    |
|                                   |                                  |                                               |
|                                   |                                  | ``CL_WFV_ERROR_CODEPLAY``. The status         |
|                                   |                                  | returned if whole function vectorization      |
|                                   |                                  | performed on the specified kernel object for  |
|                                   |                                  | the specified device generated an error.      |
|                                   |                                  |                                               |
|                                   |                                  | ``CL_WFV_SUCCESS_CODEPLAY``. The status       |
|                                   |                                  | returned if whole function vectorization      |
|                                   |                                  | performed on the specified kernel object for  |
|                                   |                                  | the specified device was successful.          |
+-----------------------------------+----------------------------------+-----------------------------------------------+
| ``CL_KERNEL_WFV_WIDTHS_CODEPLAY`` | ``size_t[]``                     | Returns a list of whole function              |
|                                   |                                  | vectorization widths for *kernel* for each    |
|                                   |                                  | work-item dimension for the specified device. |
|                                   |                                  |                                               |
|                                   |                                  | Each whole function vectorization width       |
|                                   |                                  | indicates the effective number of work items  |
|                                   |                                  | for the respective work-item dimension that   |
|                                   |                                  | will be handled for one incovation of the     |
|                                   |                                  | specified kernel. This value does not         |
|                                   |                                  | necessarily correlate to the width of the     |
|                                   |                                  | types in use by the kernel.                   |
|                                   |                                  |                                               |
|                                   |                                  | If whole function vectorization has not been  |
|                                   |                                  | performed on the specified kernel or an error |
|                                   |                                  | occurred during vectorization, the integers   |
|                                   |                                  | returned from this query will be 0 and an     |
|                                   |                                  | appropriate status value will be returned for |
|                                   |                                  | ``CL_KERNEL_WFV_STATUS_CODEPLAY`` queries.    |
+-----------------------------------+----------------------------------+-----------------------------------------------+

Version History
---------------

+---------+------------+-----------------+-------------------+
| Version | Date       | Author          | Changes           |
+=========+============+=================+===================+
| 0.1.0   | 2021/06/01 | Ori Sky Farrell | Initial proposal. |
+---------+------------+-----------------+-------------------+

.. _Kernel Object Device Queries:
   https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#kernel-workgroup-info-table

.. _clCreateProgramWithBinary:
   https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clCreateProgramWithBinary
