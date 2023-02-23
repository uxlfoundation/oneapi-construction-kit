Kernel Exec Info - ``cl_codeplay_kernel_exec_info``
===================================================

Name String
-----------

``cl_codeplay_kernel_exec_info``

Version
-------

Version 1, October 15, 2020

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

This extension adds support for allowing additional information other than
argument values to be passed to a kernel. This base extension doesn't provide
support for any particular parameter types but is intended to be built upon by
future extensions that require this support. For example Intel USM requires
support for `clSetKernelExecInfo` which is part of the 2.0 API. Instead, USM
will be able to use this extension to support 1.2.

New Types
---------

Parameter for the `execution_info` provided to `clSetKernelExecInfoCODEPLAY.
This is defined as `cl_uint` but if `cl_kernel_exec_info` is present as part of
a 2.0 or greater build we will define it to match that. 

.. code-block:: c

  typedef cl_uint cl_kernel_exec_info_codeplay;


New API Functions
-----------------

.. code-block:: c

   cl_int clSetKernelExecInfoCODEPLAY(cl_kernel kernel,
                                      cl_kernel_exec_info_codeplay param_name,
                                      size_t param_value_size,
                                      const void* param_value)


Modifications to the OpenCL API Specification
---------------------------------------------

Add a supplement to Section 5.7.2 - "Setting Kernel Arguments":
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. _clSetKernelExecInfoCODEPLAY:

To pass additional information other than argument values to a kernel, call the function

.. code-block:: c

   cl_int clSetKernelExecInfoCODEPLAY(cl_kernel kernel,
                                      cl_kernel_exec_info_codeplay param_name,
                                      size_t param_value_size,
                                      const void* param_value)

*kernel*
   specifies the kernel object being queried.

*param_name* 
   specifies the information to be passed to kernel. The supported *param_name*
   types and the corresponding values passed in for *param_value* will be
   described as part of future extensions, for example, the Intel USM
   extension.

*param_value_size*
   specifies the size in bytes of the memory pointed to by *param_value*.

*param_value*
   is a pointer to memory where the appropriate values determined by
   *param_name* are specified.


`clSetKernelExecInfoCODEPLAY`_ returns ``CL_SUCCESS`` if the function is
executed successfully. Otherwise, it returns one of the following errors:

* ``CL_INVALID_KERNEL``  if *kernel* is a not a valid kernel object.
* ``CL_INVALID_VALUE`` if *param_name* is not valid, if *param_value* is *NULL*
  or if the size specified by *param_value_size* is not valid.
* ``CL_INVALID_OPERATION`` where no devices in the context support the
  specified *param_value* and *param_name* combination.
* ``CL_OUT_OF_RESOURCES`` if there is a failure to allocate resources required
  by the OpenCL implementation on the device.
* ``CL_OUT_OF_HOST_MEMORY`` if there is a failure to allocate resources
  required by the OpenCL implementation on the host.

Revision History
----------------

+-----+------------+---------------+-------------------+
| Rev | Data       | Author        | Changes           |
+=====+============+===============+===================+
| 1   | 2020/10/15 | Mark Miller   | Initial proposal. |
+-----+------------+---------------+-------------------+
