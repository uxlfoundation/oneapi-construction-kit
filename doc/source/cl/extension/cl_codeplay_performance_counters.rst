Performance Counters - ``cl_codeplay_performance_counters``
===========================================================

Name String
-----------

``cl_codeplay_performance_counters``

Contact
-------

*  Kenneth Benzie, Codeplay Software Ltd. (k.benzie at codeplay.com)

Contributors
------------

*  Kenneth Benzie, Codeplay Software Ltd.
*  Ewan Crawford, Codeplay Software Ltd.

Version
-------

Version 1, June 17, 2019

Number
------

OpenCL Extension #XX

Status
------

Proposal

Dependencies
------------

OpenCL 1.2 and the ``cl_khr_create_command_queue`` extension are required.

Overview
--------

The performance counter extension grants the application access to performance
counters to determine the runtime characteristics of executed commands.

New API Enums
-------------

Accepted as *param_name* parameter to :opencl-1.2:`clGetDeviceInfo`:

+-----------------------------------------+--------+
| Enumeration                             | Value  |
+=========================================+========+
| CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY | 0x4260 |
+-----------------------------------------+--------+

Accepted as a key in the *properties* key value array parameter to
`clCreateCommandQueueWithPropertiesKHR`_:

+----------------------------------------+--------+
| Enumeration                            | Value  |
+========================================+========+
| CL_QUEUE_PERFORMANCE_COUNTERS_CODEPLAY | 0x4261 |
+----------------------------------------+--------+

Accepted as *param_name* parameter to :opencl-1.2:`clGetEventProfilingInfo`:

+----------------------------------------------------+--------+
| Enumeration                                        | Value  |
+====================================================+========+
| CL_PROFILING_COMMAND_PERFORMANCE_COUNTERS_CODEPLAY | 0x4262 |
+----------------------------------------------------+--------+

Accepted as *unit* member of `cl_performance_counter_codeplay`_:

+-------------------------------------------------------+-------+
| Enumeration                                           | Value |
+=======================================================+=======+
| CL_PERFORMANCE_COUNTER_UNIT_GENERIC_CODEPLAY          | 0x0   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_CODEPLAY       | 0x1   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_CODEPLAY      | 0x2   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_BYTES_CODEPLAY            | 0x3   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_CODEPLAY | 0x4   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_KELVIN_CODEPLAY           | 0x5   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_WATTS_CODEPLAY            | 0x6   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_VOLTS_CODEPLAY            | 0x7   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_AMPS_CODEPLAY             | 0x8   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_HERTZ_CODEPLAY            | 0x9   |
+-------------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_UNIT_CYCLES_CODEPLAY           | 0xA   |
+-------------------------------------------------------+-------+

Accepted as *storage* member of `cl_performance_counter_codeplay`_:

+-----------------------------------------------------+-------+
| Enumeration                                         | Value |
+=====================================================+=======+
| CL_PERFORMANCE_COUNTER_RESULT_TYPE_INT32_CODEPLAY   | 0x0   |
+-----------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_RESULT_TYPE_INT64_CODEPLAY   | 0x1   |
+-----------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_RESULT_TYPE_UINT32_CODEPLAY  | 0x2   |
+-----------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_RESULT_TYPE_UINT64_CODEPLAY  | 0x3   |
+-----------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_RESULT_TYPE_FLOAT32_CODEPLAY | 0x4   |
+-----------------------------------------------------+-------+
| CL_PERFORMANCE_COUNTER_RESULT_TYPE_FLOAT64_CODEPLAY | 0x5   |
+-----------------------------------------------------+-------+

New API Structures
------------------

.. _cl_performance_counter_codeplay:

Accepted as *param_value* parameter to :opencl-1.2:`clGetDeviceInfo`:

.. code-block:: c

   typedef struct cl_performance_counter_codeplay {
       cl_performance_counter_unit_codeplay unit;
       cl_performance_counter_storage_codeplay storage;
       cl_uint uuid;
       char name[256];
       char category[256];
       char description[256];
   } cl_performance_counter_codeplay;

.. _cl_performance_counter_desc_codeplay:

Accepted as *descs* member of `cl_performance_counter_config_codeplay`_:

.. code-block:: c

   typedef struct cl_performance_counter_desc_codeplay {
       cl_uint uuid;
       void* data;
   } cl_performance_counter_desc_codeplay;

.. _cl_performance_counter_config_codeplay:

Accepted as a value in the *properties* key value array parameter to
`clCreateCommandQueueWithPropertiesKHR`_:

.. code-block:: c

   typedef struct cl_performance_counter_config_codeplay {
       cl_uint count;
       cl_performance_counter_desc_codeplay* descs;
   } cl_performance_counter_config_codeplay;

.. _cl_performance_counter_result_codeplay:

Accepted as *param_value* parameter to :opencl-1.2:`clGetEventProfilingInfo`:

.. code-block:: c

   typedef struct cl_performance_counter_result_codeplay {
       union {
           cl_int int32;
           cl_long int64;
           cl_uint uint32;
           cl_ulong uint64;
           cl_float float32;
           cl_double float64;
       };
   } cl_performance_counter_result_codeplay;

Modifications to the OpenCL API Specification
---------------------------------------------

Add a new row to OpenCL 1.2 specification - "Table 4.3 OpenCL Device Queries"

+----------------+-------------------------------------------------------------+
| Column         | Text                                                        |
+================+=============================================================+
| cl_device_info | CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY                     |
+----------------+-------------------------------------------------------------+
| Return Type    | cl_performance_counter_codeplay[]                           |
+----------------+-------------------------------------------------------------+
| Description    | Returns a list of performance counters supported by device. |
|                | This is an array of `cl_performance_counter_codeplay`_      |
|                | structures. Each item in the list describes the unit of     |
|                | measurement, storage type, unique identifier, name,         |
|                | category, and description of a performance counter which    |
|                | can be enabled. This information is intended for use in     |
|                | profiling tools.                                            |
|                | If the device does not support performance counters         |
|                | param_value_size_ret will return a value of 0.              |
+----------------+-------------------------------------------------------------+

Add a new row to OpenCL 1.2 extension specification - "Table 9.22.1 List of
supported cl_queue_properties values and description."

+------------------+-----------------------------------------------------------+
| Column           | Text                                                      |
+==================+===========================================================+
| Queue Properties | CL_QUEUE_PERFORMANCE_COUNTERS_CODEPLAY                    |
+------------------+-----------------------------------------------------------+
| Property Value   | cl_performance_counter_config_codeplay*                   |
+------------------+-----------------------------------------------------------+
| Description      | This property value is a pointer to a                     |
|                  | `cl_performance_counter_config_codeplay`_ structure       |
|                  | containing a count specifying the number of enabled       |
|                  | counters and the descs array of                           |
|                  | `cl_performance_counter_desc_codeplay`_ structures        |
|                  | specifying which performance counters are to be enabled   |
|                  | defined by uuid the unique identifier of the counter      |
|                  | attained using the                                        |
|                  | CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY clGetDeviceInfo   |
|                  | query. The data member is an optional extension point for |
|                  | further configurability of performance counters.          |
+------------------+-----------------------------------------------------------+

Add a new row to OpenCL 1.2 specification - "Table 5.19 clGetEventProfilingInfo
parameter queries"

+-------------------+----------------------------------------------------------+
| Column            | Text                                                     |
+===================+==========================================================+
| cl_profiling_info | CL_PROFILING_COMMAND_PERFORMANCE_COUNTERS_CODEPLAY       |
+-------------------+----------------------------------------------------------+
| Return Type       | cl_performance_counter_result_codeplay[]                 |
+-------------------+----------------------------------------------------------+
| Description       | Returns a list of performance counter results. This is   |
|                   | an array of `cl_performance_counter_result_codeplay`_    |
|                   | structures, the order of results matches the order the   |
|                   | counters were enabled at command queue creation. Each    |
|                   | item in the list contains a result value in one of the   |
|                   | union members defined by the storage type of the         |
|                   | associated (has the same uuid) performance counter       |
|                   | attained using the                                       |
|                   | CL_DEVICE_PERFORMANCE_COUNTERS_CODEPLAY clGetDeviceInfo  |
|                   | query.                                                   |
|                   |                                                          |
|                   | If the event is a user event, or the command queue       |
|                   | associated with the event was not created with           |
|                   | performance counters enabled CL_INVALID_VALUE will be    |
|                   | returned.                                                |
+-------------------+----------------------------------------------------------+

Revision History

+-----+------------+----------------+------------------------------------------+
| Rev | Data       | Author         | Changes                                  |
+=====+============+================+==========================================+
| 1   | 2019/06/17 | Kenneth Benzie | Initial proposal                         |
+-----+------------+----------------+------------------------------------------+
| 2   | 2021/06/30 | Ewan Crawford  | Use enums from Codeplay reserved Khronos |
|     |            |                | range                                    |
+-----+------------+----------------+------------------------------------------+

.. _clCreateCommandQueueWithPropertiesKHR:
   https://www.khronos.org/registry/OpenCL/specs/2.2/html/OpenCL_Ext.html#cl_khr_create_command_queue
