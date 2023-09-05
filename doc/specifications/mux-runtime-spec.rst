ComputeMux Runtime Specification
================================

   This is version 0.79.0 of the specification.

ComputeMux is Codeplay’s proprietary API for executing compute workloads across
heterogeneous devices. ComputeMux is an extremely lightweight,
bare-to-the-metal abstraction on modern hardware, and is used to power a suite
of open source standards. ComputeMux consists of a :doc:`Runtime Specification
</specifications/mux-runtime-spec>` and :doc:`Compiler Specification
</specifications/mux-compiler-spec>`. This document refers to the Runtime
component.

Throughout this specification, ComputeMux has been shortened to Mux for brevity,
but for the avoidance of all doubt, "Mux" refers to "ComputeMux".

Glossary
--------

The key statements **must**, **must not**, **shall**, **shall not**,
**should**, **should not**, **may**, in this document are to be
interpreted as described in `IETF RFC
2119 <http://www.ietf.org/rfc/rfc2119.txt>`__.

-  **must** or **shall** - means that the definition is an absolute
   requirement of the specification.
-  **must not** or **shall not** - means that the definition is an
   absolute prohibition of the specification.
-  **should** - means that the definition is highly likely to occur, but
   there are extraneous circumstances that could result in it not
   occurring.
-  **should not** - means that the definition is highly likely to not
   occur, but there are extraneous circumstances that could result in it
   occurring.
-  **may** - means that an item is optional.

Introduction
------------

This document describes the Mux API, how it is used, and requirements to
follow on its usage.

Allocators
----------

All host-side allocations performed by Mux **should** use the user
provided allocators. A valid exception could be to implement Mux on top
of another framework, e.g., LLVM, which does not support user provided
allocators.

Many Mux API functions take a ``mux_allocator_info_t`` object which
specifies the allocator to use for their related object creation,
destruction, and operation of the object:

.. code:: c

   typedef struct mux_allocator_info_s mux_allocator_info_t;

Where ``mux_allocator_info_s`` is defined to be:

.. code:: c

   struct mux_allocator_info_s {
     void* (*alloc)(void* user_data, size_t size, size_t alignment);
     void (*free)(void* user_data, void* pointer);
     void* user_data;
   };

-  ``alloc`` - allocate aligned memory.
-  ``free`` - free previously allocated memory.
-  ``user_data`` - user data passed to each call of ``alloc`` and
   ``free``.

.. rubric:: Valid Usage

-  ``alloc`` **must not** be NULL, otherwise ``mux_allocator_info_s`` is
   invalid.
-  ``free`` **must not** be NULL, otherwise ``mux_allocator_info_s`` is
   invalid.
-  ``user_data`` **may** be NULL.
-  Calls to ``alloc()`` **should** return a valid pointer address, the
   address of which **must** be sized to the ``size`` argument and
   **must** be aligned to the ``alignment`` argument.
-  Calls to ``free()`` **must** succeed for any ``pointer`` argument
   that was previously returned from ``alloc()``.
-  Calls to ``alloc()`` and ``free()`` **must** be callable from
   concurrent threads of execution. Mux device’s are not required to
   lock calls to these callbacks.

Functions with a ``mux_allocator_info_t`` parameter check that the
``alloc`` and ``free`` fields are not ``NULL`` but they do not perform
exhaustive testing, e.g., it is not tested if the argument to a
``muxDestroy...`` function matches the ``mux_allocator_info_t`` argument
passed to the matching ``muxCreate...`` function.

Callbacks
---------

Mux supports a callback message reporting mechanism which allows
implementations to inform the user of detailed information relating to
their use of the API. This mechanism is optional. There are two entry
points which these callbacks **may** be supplied to,
`muxCreateFinalizer <#muxcreatefinalizer>`__ and
`muxCreateCommandBuffer <#muxcreatecommandbuffer>`__, both take a
``mux_callback_info_t`` which is defined as.

.. code:: c

   typedef struct mux_callback_info_s* mux_callback_info_t;

Where ``mux_callback_info_s`` is defined to be:

.. code:: c

   struct mux_callback_info_s {
     void (*callback)(
         void* user_data,
         const char* message,
         const void* data,
         size_t data_size);
     void *user_data;
   };

-  ``callback`` - callback to provide detailed information about an
   error or to provide optimization hints to the user.

   -  ``user_data`` - Pointer to the user data provided by the user.
   -  ``message`` - String containing a message from the implementation
      to the user.
   -  ``data`` - An implementation defined binary data blob containing
      additional data, **may** be null.
   -  ``data_size`` - The size in bytes of the data blob, **must** be
      ``0`` if ``data`` is NULL.

-  ``user_data`` - User data provided by the user.

.. rubric:: Valid Usage

-  ``callback`` **must not** be NULL, otherwise ``mux_callback_info_s``
   is invalid.
-  ``user_data`` **may** be NULL.

Devices
-------

Mux is designed to support multiple devices. Each device conforms to the
same API, but is allowed to expose differing capabilities. For example,
device A could support images, whereas device B might not.

There are multiple possible types of device which are distinguished by
the ``mux_device_type_e`` enumeration.

.. code:: c

   enum mux_device_type_e {
     mux_device_type_cpu = (0x1 << 0),
     mux_device_type_gpu_integrated = (0x1 << 1),
     mux_device_type_gpu_discrete = (0x1 << 2),
     mux_device_type_gpu_virtual = (0x1 << 3),
     mux_device_type_accelerator = (0x1 << 4),
     mux_device_type_custom = (0x1 << 5),
     mux_device_type_compiler = (0x1 << 6),
     mux_device_type_all = 0xFFFFFFFF
   };

-  ``mux_device_type_cpu`` - device is a CPU.
-  ``mux_device_type_gpu_integrated`` - device is an integrated GPU.
-  ``mux_device_type_gpu_discrete`` - device is a discrete GPU.
-  ``mux_device_type_gpu_virtual`` - device is a virtualized GPU.
-  ``mux_device_type_accelerator`` - device is an accelerator chip.
-  ``mux_device_type_custom`` - device is a mysterious custom type.
-  ``mux_device_type_compiler`` - device is a compiler, used for offline
   and cross compilation.
-  ``mux_device_type_all`` - bit-mask to match all device types.

An instance of ``mux_device_info_t`` contains the information about a
device’s capabilities and related metadata, and ``mux_device_t`` stores
an initialized device’s state. A device for which ``mux_device_info_t``
**may** not be initializable, for example if it’s not physically present
in the system, but it can still be useful as a target for compilation.

.. code:: c

   struct mux_device_info_s {
     mux_id_t id;
     uint32_t allocation_capabilities;
     uint32_t source_capabilities;
     uint32_t address_capabilities;
     uint32_t cache_capabilities;
     uint32_t half_capabilities;
     uint32_t float_capabilities;
     uint32_t double_capabilities;
     uint32_t integer_capabilities;
     uint32_t custom_buffer_capabilities;
     uint32_t endianness;
     uint32_t khronos_vendor_id;
     uint32_t shared_local_memory_type;
     uint32_t device_type;
     const char* builtin_kernel_declarations;
     const char* device_name;
     uint32_t max_concurrent_work_items;
     uint32_t max_work_group_size_x;
     uint32_t max_work_group_size_y;
     uint32_t max_work_group_size_z;
     uint32_t max_work_width;
     uint32_t clock_frequency;
     uint32_t compute_units;
     uint32_t buffer_alignment;
     uint64_t memory_size;
     uint64_t allocation_size;
     uint64_t cache_size;
     uint64_t cacheline_size;
     uint64_t shared_local_memory_size;
     uint32_t native_vector_width;
     uint32_t preferred_vector_width;
     bool image_support;
     bool image2d_array_writes;
     bool image3d_writes;
     uint32_t max_image_dimension_1d;
     uint32_t max_image_dimension_2d;
     uint32_t max_image_dimension_3d;
     uint32_t max_image_array_layers;
     uint32_t max_storage_images;
     uint32_t max_sampled_images;
     uint32_t max_samplers;
     uint32_t queue_types[mux_queue_type_total];
     int8_t device_priority;
     bool query_counter_support;
     bool descriptors_updatable;
     bool can_clone_command_buffers;
     bool supports_builtin_kernels;
     uint32_t max_sub_group_count;
     bool sub_groups_support_ifp;
     uint32_t max_hardware_counters;
     bool supports_work_group_collectives;
     bool supports_generic_address_space;
   };

-  ``id`` - the ID of this device object.
-  ``allocation_capabilities`` - a bitfield of
   ``mux_allocation_capabilities_e``.
-  ``source_capabilities`` - a bitfield of
   ``mux_source_capabilities_e``.
-  ``address_capabilities`` - a bitfield of
   ``mux_address_capabilities_e``.
-  ``cache_capabilities`` - a bitfield of ``mux_cache_capabilities_e``.
-  ``half_capabilities`` - half floating point support, a bitfield of
   ``mux_floating_point_capabilities_e``.
-  ``float_capabilities`` - floating point support, a bitfield of
   ``mux_floating_point_capabilities_e``.
-  ``double_capabilities`` - double floating point support, a bitfield
   of ``mux_floating_point_capabilities_e``.
-  ``integer_capabilities`` - integer support, a bitfield of
   ``mux_integer_capabilities_e``.
-  ``custom_buffer_capabilities`` - custom buffer support, a bitfield of
   ``mux_custom_buffer_capabilities_e``.
-  ``endianness`` - endianness of the device, one of
   ``mux_endianness_e``.
-  ``khronos_vendor_id`` - the Khronos vendor ID of this device.
-  ``shared_local_memory_type`` - the type of shared local memory this
   device has, one of ``mux_shared_local_memory_type_e``.
-  ``device_type`` - the type of this device, one of
   ``mux_device_type_e``.
-  ``builtin_kernel_declarations`` - a null-terminated C string of
   semicolon-separated built-in kernel declarations.
-  ``device_name`` - the string name of this device, terminated by a
   ``\0``.
-  ``max_concurrent_work_items`` - the maximum number of work items in a
   work group that can run concurrently.
-  ``max_work_group_size_x`` - the maximum number of work items in the x
   dimension of a work group.
-  ``max_work_group_size_y`` - the maximum number of work items in the y
   dimension of a work group.
-  ``max_work_group_size_z`` - the maximum number of work items in the z
   dimension of a work group.
-  ``max_work_width`` - the maximum number of work items of a work group allowed
   to execute in one invocation of a kernel.
-  ``clock_frequency`` - the maximum normal clock frequency (in MHz) of
   this device.
-  ``compute_units`` - the number of compute units this device has.
-  ``buffer_alignment`` the alignment (in bytes) of ``mux_buffer_t``\ ’s
   allocated by this device.
-  ``memory_size`` - the size (in bytes) of the memory on this device.
-  ``allocation_size`` - the maximum size (in bytes) of a single memory
   allocation on this device.
-  ``cache_size`` - the size (in bytes) of the cache on this device.
   ``cache_size`` **may** be zero if the device does not support a
   cache.
-  ``cacheline_size`` - the size (in bytes) of a line on the cache on
   this device. ``cacheline_size`` **may** be zero if the device does
   not support a cache.
-  ``shared_local_memory_size`` - the size (in bytes) of the shared
   local memory on this device.
-  ``native_vector_width`` - the native vector width (in bytes).
-  ``preferred_vector_width`` - the preferred vector width (in bytes).
-  ``image_support`` - Is true if the device supports images otherwise
   false.
-  ``image2d_array_writes`` - If the device supports 2d image array
   writes.
-  ``image3d_writes`` - If the device supports 3d image writes.
-  ``max_image_dimension_1d`` - The maximum dimension size in pixels for
   a one dimensional image, zero if images are not supported.
-  ``max_image_dimension_2d`` - The maximum dimension size in pixels for
   a two dimensional image, zero if images are not supported.
-  ``max_image_dimension_3d`` - The maximum dimension size in pixels for
   a three dimensional image, zero if images are not supported.
-  ``max_image_array_layers`` - The maximum array layers count for an
   image array, zero if images are not supported.
-  ``max_storage_images`` - The maximum number of bound images for image
   storage (write), zero if images are not supported.
-  ``max_sampled_images`` - The maximum number of bound images for image
   sampling (read), zero if images are not supported.
-  ``max_samplers`` - The maximum number of bound samplers, zero if
   images are not supported.
-  ``queue_types`` - An array of queue types - one for each
   ``mux_queue_type_e`` member. Each element of the array specifies how
   many queues of a specific type are supported by the device.
-  ``device_priority`` - This value is used for tracking device priority
   when deciding which devices shall be returned if
   ``CL_DEVICE_TYPE_DEFAULT`` is requested. Host will have a value of
   ``0`` where higher priority devices will be in the positive range and
   conversely a lower priority for the negative range.
-  ``query_counter_support`` - Is true if the device supports
   ``mux_query_type_counter``.
-  ``descriptors_updatable`` -  Is true the device supports updating the
   ``mux_device_info_t``  argument descriptors passed to a specialized
   kernel in a ``muxCommandNDRange`` command after the containing
   ``mux_command_buffer`` has been finalized.
-  ``can_clone_command_buffers`` - Is true if the device supports cloning
   ``mux_command_buffers`` via the ``muxCloneCommandBuffer`` entry point.
-  ``supports_builtin_kernels`` - Is true if the device supports creating
   built-in kernels via the ``muxCreateBuiltInKernel`` entry point.
-  ``max_sub_group_count`` - The maximum number of sub-groups in a
   work-group. A target not supporting sub-groups must set this to `0`.
- ``sub_groups_support_ifp`` - Is true if the device supports independent
  forward progress in its sub-groups. A target not supporting sub-groups
  must set this to `false`.
- ``max_hardware_counters`` - Maximum number of hardware counters that
  can be active in query pools at one time.
- ``supports_work_group_collectives`` - Is true if the device supports
  work-group collective Mux builtins. A target not supporting work-group
  collectives **must** set this to false.
- ``supports_generic_address_space`` - Is true if the device supports the
  Generic Address Space. A target not supporting the Generic Address Space
  **must** set this to false.

.. rubric:: Valid Usage

-  ``allocation_capabilities`` **must** have at least one of
   ``mux_allocation_capabilities_coherent_host`` or
   ``mux_allocation_capabilities_alloc_device`` bits set.
-  ``allocation_size`` **must** be greater than ``0`` if ``memory_size``
   is greater than ``0``.
-  ``allocation_size`` **must** be less than or equal to ``memory_size``
-  ``compute_units`` **must** be greater than ``0``.
-  If the ``mux_source_capabilities_builtin_kernel`` bit of
   ``source_capabilities`` is set, then ``builtin_kernel_declarations``
   **must** point to a null-terminated C string of semicolon-separated
   built-in kernel *declarations*.
-  Built-in kernel *declarations* **must** conform to the `built-in
   kernel declaration syntax <#built-in-kernel-declaration-syntax>`__,
   defined below.

Built-In Kernel Declaration Syntax
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Built-in kernel *declarations* **must** be of the form
``kernel_name(parameters)``. The declarations conform to the OpenCL 1.2
kernel declaration syntax with the following exceptions:

1. The leading ``__kernel`` keyword **must not** be used
2. The return type (``void``) **must** be omitted
3. Pointer parameters using ``[]`` notation **must not** be used
4. Struct parameters and pointer-to-struct parameters **must not** be
   used
5. Type and variable attributes (e.g., ``__attribute__*``) **must not**
   be used
6. All whitespace **must** only be `` `` characters; other whitespace
   characters (``\t``, ``\n``, etc.) **must not** be used
7. The built-in kernel name including a trailing NUL character must not
   exceed ``CL_NAME_VERSION_KHR`` (64 characters).

Also note that two OpenCL limitations are present for built-in kernel
parameter:

1. OpenCL kernels do not support pointer-to-pointer parameters
2. The OpenCL API ignores ``const`` and ``volatile`` qualifiers on value
   parameters. These **may** or **may not** be reported. (The OpenCL C
   compiler will not ignore the qualifiers.)

This is an example of a valid list of built-in kernel declarations:

.. code:: c

   mux_device_info_s_ptr->builtin_kernel_declarations =
     "kernel1(global const int* in, global int* out);"
     "kernel2(read_only image3d_t in, read_write image2d_t out, float val);"
     "kernel3(local const float* restrict in, __global volatile float* out)";
   };

muxGetDeviceInfos
~~~~~~~~~~~~~~~~~

``muxGetDeviceInfos()`` calls into Mux to enumerate device for which
information is registered with Mux.

.. code:: c

   mux_result_t muxGetDeviceInfos(
       uint32_t device_types,
       uint64_t device_infos_length,
       mux_device_info_t *out_device_infos,
       uint64_t *out_device_infos_length);

-  ``device_types`` - a bitfield of ``mux_device_type_e`` specifying the
   types of devices to retrieve the device information from,
   ``mux_device_type_all`` can be used to specify all devices.
-  ``device_infos_length`` - the length of ``out_device_infos``.
-  ``out_device_infos`` - an array of Mux devices information.
-  ``out_device_infos_length`` - the total number of devices for which
   information was recorded in ``out_device_infos``.

``muxGetDeviceInfos()`` can be called in two configurations.

.. code:: c

   uint64_t device_infos_length = 0;
   mux_result_t error =
       muxGetDeviceInfos(mux_device_type_all, 0, NULL, &device_infos_length);

This first configuration sets ``device_infos_length`` to 0,
``out_device_infos`` to NULL, and passes a valid pointer for
``out_device_infos_length`` - on success ``out_device_infos_length``
will contain the total number of devices for which information is
recorded in Mux.

.. code:: c

   mux_device_info_t *device_infos =
       malloc(sizeof(mux_device_info_t) * device_infos_length);
   error = muxGetDeviceInfos(mux_device_type_all, device_infos_length,
                              device_infos, 0);

This second configuration sets ``device_infos_length`` to a non-zero
number, ``out_device_infos`` to a non-NULL pointer, and passes NULL for
``out_device_infos_length`` - on success ``out_device_infos`` will
contain ``device_infos_length`` number of valid Mux device information
entries to use.

The returned ``mux_device_info_t``\ s have static lifetimes, and **must
not** require any destruction logic.

.. rubric:: Return Codes

-  If ``device_types`` is 0, ``mux_error_invalid_value`` **must**
   be returned.
-  If ``device_infos_length`` is 0 and ``out_device_infos`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``device_infos_length`` is not 0 and ``out_device_infos`` is NULL,
   ``mux_error_null_out_parameter`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned, all
``out_device_infos`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxGetDeviceInfos()`` **shall** be considered thread-safe.

muxGetDeviceInfos_t hook
~~~~~~~~~~~~~~~~~~~~~~~~

Related to ``muxGetDeviceInfos`` is the device information hook, which
**must** be implemented by each of the target implementations in order
to support enumerating devices owned by that implementation. A function
signature for a target ``target`` would look as follows:

.. code:: c

   mux_result_t targetGetDeviceInfos(
       uint32_t device_types,
       uint64_t device_infos_length,
       mux_device_info_t* out_device_infos,
       uint64_t* out_device_infos_length);

The arguments have a very similar meaning as the ones of
``muxGetDeviceInfos``, with the difference that it only needs to handle
its own device list, it doesn’t know about other devices registered in
the system. The hook should support being called with a null
``out_device_infos`` argument to get the number of devices it wants to
register. It should also support being called with
``device_infos_length`` set to that number to populate the
``out_device_infos`` array.

muxCreateDevices
~~~~~~~~~~~~~~~~

.. code:: c

   struct mux_device_s {
     mux_id_t id;
     mux_device_info_t info;
   };

``muxCreateDevices()`` calls into Mux to create and initialize the list
of devices registered with Mux.

.. code:: c

   mux_result_t muxCreateDevices(
       uint64_t devices_length,
       mux_device_info_t *device_infos,
       mux_allocator_info_t allocator_info,
       mux_device_t *out_devices);

-  ``devices_length`` - the total number of devices to initialize, the
   length of the ``device_infos`` and ``out_devices`` arrays.
-  ``device_infos`` - array of device information determining which
   devices to create, the entries **must** have come from a
   ``muxGetDeviceInfos()`` call.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`_ section.
-  ``out_devices`` - the newly created devices.

Example usage:

.. code:: c

   uint64_t chosen_devices_count = ...;
   mux_device_info_t *chosen_device_infos = ...;
   mux_allocator_info_t allocator = ...;
   mux_device_t *devices = NULL;
   error = muxCreateDevices(chosen_devices_count, chosen_device_infos,
                             allocator, devices);

.. rubric:: Return Codes

-  If ``devices_length`` is greater than 0 and ``device_infos`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``devices_length`` is greater than 0 and ``out_devices`` is NULL,
   ``mux_error_null_out_parameter`` **must** be returned.
-  If at least one of the ``device_infos`` represents a device that is
   not available for initialization (e.g. if it’s only a target for
   cross-compilation), ``mux_error_feature_unsupported`` **must** be
   returned.
-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_devices`` is ``NULL``, then ``mux_error_null_out_parameter``
   **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned, all of
the ``out_devices`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxGetDeviceInfos()`` **shall not** be considered
   thread-safe.
-  If any of the ``device_infos`` did not originate from a call to
   ``muxGetDeviceInfos()``, the behavior is undefined.
-  The ``device_infos`` array can contain less elements than the one
   provided by ``muxGetDeviceInfos()``, but their order **must** be
   maintained. Otherwise the behavior is undefined.

muxCreateDevices_t hook
~~~~~~~~~~~~~~~~~~~~~~~

Related to ``muxCreateDevices`` is the device creation hook, which
**must** be implemented by each of the target implementations in order
to support creating devices owned by that implementation. A function
signature for a target ``target`` would look as follows:

.. code:: c

   mux_result_t targetCreateDevices(
       uint64_t devices_length,
       mux_device_info_t* device_infos,
       mux_allocator_info_t allocator,
       mux_device_t* out_devices);

The arguments have the same meaning as the ones of ``muxCreateDevices``.
The hook should iterate over the ``device_infos`` array, and create its
own device objects at the indices corresponding to info object entries
managed by that target. The hook can also return an error code to
signify failure.

The arguments have a very similar meaning as the ones of
``muxCreateDevices``, with the difference that a hook only needs to
handle its own device list, it doesn’t know about other devices
registered in the system.

A device creation hook **must** not check for the errors described by
the ``muxCreateDevices`` return code section as these are detected and
handled before the hook is called.

muxDestroyDevice
~~~~~~~~~~~~~~~~

``muxDestroyDevice()`` destroys a previously created ``mux_device_t``.
Note that ``muxDestroyDevice()`` destroys a single device only - If
``muxCreateDevices()`` returned multiple devices, each has to be
destroyed individually.

.. code:: c

   void muxDestroyDevice(
       mux_device_t device,
       mux_allocator_info_t allocator_info);

-  ``device`` - a device previously created by a call to
   ``muxCreateDevices()``.
-  ``allocator_info`` - the user provided allocator ``device`` was
   created with.

.. rubric:: Valid Usage

-  The ``allocator_info`` passed to ``muxDestroyDevice()`` **must** be
   the same ``mux_allocator_info_t`` as was used in the call to
   ``muxCreateDevices()`` that created ``device``.
-  Calls to ``muxDestroyDevice()`` with a given ``device`` **shall** be
   considered thread-safe.
-  All ``mux_queue_t``\ ’s belonging to a given ``device`` **must not**
   be used once the call to ``muxDestroyDevice()`` has completed.
-  All ``mux_command_buffer_t``\ ’s dispatched to ``mux_queue_t``\ ’s
   that belong to a given ``device`` **must** have completed before a
   call to ``muxDestroyDevice()``.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

Executables
-----------

Executables in Mux are created from a binary representation compatible with the
current device. Executables **may** contain multiple entry points.

A Mux executable will have one or more named kernels.

muxCreateExecutable
~~~~~~~~~~~~~~~~~~~

``muxCreateExecutable()`` creates an executable from a given binary.

.. code:: c

   mux_result_t muxCreateExecutable(
       mux_device_t device,
       const void* binary,
       uint64_t binary_length,
       mux_allocator_info_t allocator_info,
       mux_executable_t* out_executable);

-  ``device`` - the device to create the executable with.
-  ``binary`` - an array containing the binary to load.
-  ``binary_length`` - the length, in bytes, of ``binary``.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_executable`` - the newly created executable.

.. rubric:: Return Codes

-  If ``binary`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``binary_length`` is 0, ``mux_error_invalid_value`` **must**
   be returned.
-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_executable`` is NULL, ``mux_error_null_out_parameter``
   **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_executable`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxCreateExecutable()`` **shall** be considered thread-safe.

muxDestroyExecutable
~~~~~~~~~~~~~~~~~~~~

``muxDestroyExecutable()`` destroys a previously created
``mux_executable_t``.

.. code:: c

   void muxDestroyExecutable(
       mux_device_t device,
       mux_executable_t executable,
       mux_allocator_t allocator_info);

-  ``device`` - the device the executable was created with.
-  ``executable`` - an executable previously created by a call to
   ``muxCreateExecutable()``.
-  ``allocator_info`` - the user provided allocator ``executable`` was
   created with.

.. rubric:: Valid Usage

-  Calls to ``muxDestroyExecutable()`` with a given ``executable``
   **shall** be considered thread-safe.
-  The ``allocator_info`` passed to ``muxDestroyExecutable()`` **must**
   be the same ``mux_allocator_info_t`` as was used in the call to
   ``muxCreateExecutable()`` that created ``executable``.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``executable`` **must** be a valid ``mux_executable_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

.. rubric:: Validation Rules

-  The ``mux_device_t``, that was used in the creation of a
   ``mux_executable_t`` **must not** be destroyed before the executable
   has been destroyed.
-  If the ``binary`` that was used in the creation of a ``mux_executable_t`` was
   returned by ``compiler::Kernel::createSpecializedKernel``, the
   ``compiler::Kernel`` object **must not** be destroyed before the executable
   has been destroyed.

Descriptors
-----------

Executables that run on a Mux device are passed data from the host via
``mux_descriptor_info_t``\ ’s. As the name suggests, descriptors specify
what the data for a given parameter is. When descriptors are passed to a
Mux device a copy is made of the data contained within, once the entry
point consuming the descriptors completes the storage can be reused or
freed.

.. code:: c

   struct mux_descriptor_info_s {
     uint32_t type;
     union {
       struct mux_descriptor_info_buffer_s buffer_descriptor;
       struct mux_descriptor_info_image_s image_descriptor;
       struct mux_descriptor_info_sampler_s sampler_descriptor;
       struct mux_descriptor_info_plain_old_data_s plain_old_data_descriptor;
       struct mux_descriptor_info_shared_local_buffer_s
           shared_local_buffer_descriptor;
       struct mux_descriptor_info_custom_buffer_s custom_buffer_descriptor;
     };
   };

-  ``type`` - the type of this descriptor, one of
   ``mux_descriptor_info_type_e``.
-  ``buffer_descriptor`` - the description of the buffer.
-  ``image_descriptor`` - the description of the image.
-  ``sampler_descriptor`` - the description of the sampler.
-  ``plain_old_data_descriptor`` - the description of the plain old
   data.
-  ``shared_local_buffer_descriptor`` - the description of the shared
   local buffer.
-  ``custom_buffer_descriptor`` - the description of the custom buffer.

.. rubric:: Valid Usage

-  If ``type`` is ``mux_descriptor_info_type_buffer``

   -  ``buffer_descriptor`` **must** be initialized.
   -  ``image_descriptor``, ``sampler_descriptor``,
      ``plain_old_data_descriptor``, ``shared_local_buffer_descriptor``,
      and ``custom_buffer_descriptor`` **must not** be initialized.

-  If ``type`` is ``mux_descriptor_info_type_image``

   -  ``image_descriptor`` **must** be initialized.
   -  ``buffer_descriptor``, ``sampler_descriptor``,
      ``plain_old_data_descriptor``, ``shared_local_buffer_descriptor``,
      and ``custom_buffer_descriptor`` **must not** be initialized.

-  If ``type`` is ``mux_descriptor_info_type_sampler``

   -  ``sampler_descriptor`` **must** be initialized.
   -  ``buffer_descriptor``, ``image_descriptor``,
      ``plain_old_data_descriptor``, ``shared_local_buffer_descriptor``,
      and ``custom_buffer_descriptor`` **must not** be initialized.

-  If ``type`` is ``mux_descriptor_info_type_plain_old_data``

   -  ``plain_old_data_descriptor`` **must** be initialized.
   -  ``buffer_descriptor``, ``image_descriptor``,
      ``sampler_descriptor``, ``shared_local_buffer_descriptor``, and
      ``custom_buffer_descriptor`` **must not** be initialized.

-  If ``type`` is ``mux_descriptor_info_type_shared_local_buffer``

   -  ``shared_local_buffer_descriptor`` **must** be initialized.
   -  ``buffer_descriptor``, ``image_descriptor``,
      ``sampler_descriptor``, ``plain_old_data_descriptor``, and
      ``custom_buffer_descriptor`` **must not** be initialized.

-  If ``type`` is ``mux_descriptor_info_type_null_buffer``

   -  ``buffer_descriptor``, ``image_descriptor``,
      ``sampler_descriptor``, ``plain_old_data_descriptor``,
      ``shared_local_buffer_descriptor``, and
      ``custom_buffer_descriptor`` **must not** be initialized.

-  If ``type`` is ``mux_descriptor_info_type_custom_buffer``

   -  ``custom_buffer_descriptor`` **must** be initialized.
   -  ``buffer_descriptor``, ``image_descriptor``,
      ``sampler_descriptor``, ``plain_old_data_descriptor``, and
      ``shared_local_buffer_descriptor`` **must not** be initialized.

Buffer Descriptors
~~~~~~~~~~~~~~~~~~

Buffer descriptors specify that a given ``mux_descriptor_info_t`` is a
``mux_descriptor_info_buffer_s``.

Buffer descriptors specify that the descriptor contains a
``mux_buffer_t`` for reading/writing on a device.

The ``mux_descriptor_info_type_buffer`` enumeration of
``mux_descriptor_info_type_e`` is used to specify a buffer descriptor.

.. code:: c

   struct mux_descriptor_info_buffer_s {
     mux_buffer_t buffer;
     uint64_t offset;
   };

-  ``buffer`` - the buffer that this descriptor is *describing*.
-  ``offset`` - the offset into ``buffer`` to begin indexing.

Image Descriptors
~~~~~~~~~~~~~~~~~

Image descriptors specify that a given ``mux_descriptor_info_t`` is a
``mux_descriptor_info_image_s``.

Image descriptors specify that the descriptor contains a ``mux_image_t``
for reading/writing on a device.

The ``mux_descriptor_info_type_image`` enumeration of
``mux_descriptor_info_type_e`` is used to specify an image descriptor.

.. code:: c

   struct mux_descriptor_info_image_s {
     mux_image_t image;
   };

-  ``image`` - the image that this descriptor is *describing*.

Sampler Descriptors
~~~~~~~~~~~~~~~~~~~

Sampler descriptors specify that a given ``mux_descriptor_info_t`` is a
``mux_descriptor_info_sampler_s``.

Sampler descriptors specify that the descriptor contains a
``mux_sampler_t`` for using in conjunction with a ``mux_image_t`` for
reading/writing on a device.

The ``mux_descriptor_info_type_sampler`` enumeration of
``mux_descriptor_info_type_e`` is used to specify a sampler descriptor.

.. code:: c

   struct mux_descriptor_info_sampler_s {
     mux_sampler_t sampler;
   };

-  ``sampler`` - the sampler that this descriptor is *describing*.

Plain Old Data Descriptors
~~~~~~~~~~~~~~~~~~~~~~~~~~

Plain-old-data descriptors specify that a given
``mux_descriptor_info_t`` is a ``mux_descriptor_info_plain_old_data_s``.

Plain-old-data descriptors specify that the descriptor contains a C type
or struct that is being passed purely as data on a device.

The ``mux_descriptor_info_type_plain_old_data`` enumeration of
``mux_descriptor_info_type_e`` is used to specify a buffer descriptor.

.. code:: c

   struct mux_descriptor_info_plain_old_data_s {
     const void* data;
     size_t length;
   };

-  ``data`` - the data that this descriptor is *describing*.
-  ``offset`` - the length (in bytes) of ``data``.

Shared Local Buffer Descriptors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Shared local buffer descriptors specify that a given
``mux_descriptor_info_t`` is a
``mux_descriptor_info_shared_local_buffer_s``.

Shared local buffer descriptors specify that the descriptor is
specifying that a buffer should be created solely for sharing data
between work-items in a workgroup.

The ``mux_descriptor_info_type_shared_local_buffer`` enumeration of
``mux_descriptor_info_type_e`` is used to specify a buffer descriptor.

.. code:: c

   struct mux_descriptor_info_shared_local_buffer_s {
     size_t size;
   };

-  ``size`` - the size (in bytes) of shared local memory being
   requested.

Null Descriptors
~~~~~~~~~~~~~~~~

Null descriptors specify that a given ``mux_descriptor_info_t``
specifies a NULL pointer.

Null descriptors specify that the descriptor is specifying that some
pointer is NULL.

The ``mux_descriptor_info_type_null_buffer`` enumeration of
``mux_descriptor_info_type_e`` is used to specify a null descriptor.

Custom Buffer Descriptors
~~~~~~~~~~~~~~~~~~~~~~~~~

Custom buffer descriptors specify that a given ``mux_descriptor_info_t``
is a ``mux_descriptor_info_custom_buffer_s``.

Custom buffer descriptors specify that the descriptor contains data to
enable targets to support additional information which **may** be used
to implement target specific extensions.

The ``mux_descriptor_info_type_custom_buffer`` enumeration of
``mux_descriptor_info_type_e`` is used to specify a custom buffer
descriptor.

.. code:: c

   struct mux_descriptor_info_custom_buffer_s {
     void *data;
     size_t size;
     uint32_t address_space;
   };

-  ``data`` - the optional pointer to arbitrary custom buffer data,
   **must** be set when
   ``mux_device_info_s::custom_buffer_capabilities`` contains the
   ``mux_custom_buffer_capabilities_data`` flag.
-  ``size`` - the optional size in bytes of the memory pointed to by
   ``data``, **must** be set when
   ``mux_device_info_s::custom_buffer_capabilities`` contains the
   ``mux_custom_buffer_capabilities_data`` flag.
-  ``address_space`` - the optional address space of the custom buffer,
   **must** be set when
   ``mux_device_info_s::custom_buffer_capabilities`` contains the
   ``mux_custom_buffer_capabilities_address_space`` flag.

Kernels
-------

Kernels in Mux are created from a ``mux_executable_t``. Kernels
represent a single, named, entry point into an executable. Kernels
contain fields called ``preferred_local_size_x``,
``preferred_local_size_y``, and ``preferred_local_size_z`` which can be
used by the implementation to signal preferred defaults for when a kernel is
compiled.

.. code:: c

   struct mux_kernel_s {
     mux_id_t id;
     mux_device_t device;
     size_t preferred_local_size_x;
     size_t preferred_local_size_y;
     size_t preferred_local_size_z;
     size_t local_memory_size;
     size_t max_sub_group_count;
   };

-  ``id`` - the ID of this kernel object.
-  ``device`` - the device this kernel object was created from
-  ``preferred_local_size_x`` - the preferred local size in the x
   dimension for this kernel object
-  ``preferred_local_size_y`` - the preferred local size in the y
   dimension for this kernel object
-  ``preferred_local_size_z`` - the preferred local size in the z
   dimension for this kernel object
-  ``local_memory_size`` - the amount of local memory used by this
   kernel object
-  ``max_sub_group_count`` - the maximum number of sub-groups this kernel can
   support when enqueued.

muxCreateKernel
~~~~~~~~~~~~~~~

``muxCreateKernel()`` creates a kernel from a given Mux executable.

.. code:: c

   mux_result_t muxCreateKernel(
       mux_device_t device,
       mux_executable_t executable,
       const char* name,
       uint64_t name_length,
       mux_allocator_info_t allocator_info,
       mux_kernel_t* out_kernel);

-  ``device`` - the device to create the kernel with.
-  ``executable`` - the ``mux_executable_t`` to find our kernel within.
-  ``name`` - the name of the kernel-function to select from the
   ``executable``.
-  ``name_length`` - the string length of ``name``.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_kernel`` - the newly created kernel.

.. rubric:: Return Codes

-  If ``name`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If a kernel called ``name`` was not found in ``executable``,
   ``mux_error_missing_kernel`` **must** be returned.
-  If ``name_length`` is 0, ``mux_error_invalid_value`` **must**
   be returned.
-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_kernel`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_kernel`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxCreateKernel()`` **shall** be considered thread-safe.

muxCreateBuiltInKernel
~~~~~~~~~~~~~~~~~~~~~~

``muxCreateBuiltInKernel()`` creates a kernel from a given built-in kernel name.

.. code:: c

   mux_result_t muxCreateBuiltInKernel(
       mux_device_t device,
       const char* name,
       uint64_t name_length,
       mux_allocator_info_t allocator_info,
       mux_kernel_t* out_kernel);

-  ``device`` - the device to create the kernel with.
-  ``name`` - the name of the built-in kernel-function to select from the
   ``device``.
-  ``name_length`` - the string length of ``name``.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_kernel`` - the newly created kernel.

.. rubric:: Return Codes

-  If ``name`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If a built-in kernel called ``name`` is not in
   ``mux_device_info_s::builtin_kernel_declarations``, then
   ``mux_error_missing_kernel`` **must** be returned.
-  If ``name_length`` is 0, ``mux_error_invalid_value`` **must**
   be returned.
-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_kernel`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_kernel`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxCreateBuiltInKernel()`` **shall** be considered thread-safe.

muxQuerySubGroupSizeForLocalSize
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``muxQuerySubGroupSizeForLocalSize`` queries a ``mux_kernel_t`` for the
sub-group size that would result from an enqueue of the kernel with the given
local size.

Entry point is optional and **must** return ``mux_error_feature_unsupported`` if
it is not supported.

.. code:: c

   mux_result_t muxQuerySubGroupSizeForLocalSize(mux_kernel_t kernel,
                                                 size_t local_size_x,
                                                 size_t local_size_y,
                                                 size_t local_size_z,
                                                 size_t *out_sub_group_size);

-  ``kernel`` - the kernel to query the sub-group size for.
-  ``local_size_x`` - the local size in the x dimension of the potential enqueue.
-  ``local_size_y`` - the local size in the y dimension of the potential enqueue.
-  ``local_size_z`` - the local size in the z dimension of the potential enqueue.
-  ``out_sub_group_size`` - the sub-group size that would result from a kernel
   enqueue with the specified local size.

.. rubric:: Return Codes

- If ``kernel`` is NULL, ``mux_error_invalid_value`` **must** be returned.
- If ``local_size_x`` is 0 ``mux_error_invalid_value`` **must** be returned.
- If ``local_size_y`` is 0 ``mux_error_invalid_value`` **must** be returned.
- If ``local_size_y`` is 0 ``mux_error_invalid_value`` **must** be returned.
- If ``out_sub_group_size`` is NULL, ``mux_error_null_out_parameter`` **must**
  be returned.
- If the device to which the kernel will be enqueued does not support
  sub-groups i.e. ``kernel->device->info->max_sub_group_count == 0``,
  ``mux_error_feature_unsupported`` **must** be returned.

If an error code other than ``mux_success`` is returned, the value at
``out_sub_group_size`` **must not** be considered valid.

.. rubric:: Valid Usage

-  Calls to ``muxQuerySubGroupSizeForLocalSize()`` **shall** be considered
   thread-safe.
-  Enqueuing ``kernel`` with the specified local size **shall** result in at
   least one sub-group of the size returned in ``out_sub_group_size`` and **may**
   additionally result in exactly one sub-group of size less than the returned
   when the local size is not evenly divisible by the sub-group size.

muxQueryLocalSizeForSubGroupCount
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``muxQueryLocalSizeForSubGroupCount()`` queries the local size that would
specified for an enqueue of the given kernel would result in the given number
of sub-groups.

Entry point is optional and **must** return ``mux_error_feature_unsupported`` if
it is not supported.

.. code:: c

   mux_result_t muxQueryLocalSizeForSubGroupCount(mux_kernel_t kernel,
                                                  size_t sub_group_count,
                                                  size_t *out_local_size_x,
                                                  size_t *out_local_size_y,
                                                  size_t *out_local_size_z);

-  ``kernel`` - the kernel to query the local size for.
-  ``sub_group_count`` - the requested number of sub-groups for the kernel enqueue.
-  ``out_local_size_x`` - the local size in the x dimension that would result
   in ``sub_group_count`` sub-groups.
-  ``out_local_size_y`` - the local size in the y dimension that would result
   in ``sub_group_count`` sub-groups.
-  ``out_local_size_z`` - the local size in the z dimension that would result
   in ``sub_group_count`` sub-groups.

The local size returned through the output **must** be 1 dimensional, that is
at least two of ``out_local_size_x``, ``out_local_size_y`` and
``out_local_size_z`` must be 1. The local size **must** be exactly divisible by
the sub-group size for ``kernel`` with returned local size with no remainder.
The local size returned may be zero, indicating no local size can support the
requested number of sub-groups.

.. rubric:: Return Codes

-  If ``kernel`` is NULL, ``mux_error_invalid_value`` **must** be returned.
-  If ``out_local_size_x`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  If ``out_local_size_y`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  If ``out_local_size_z`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  If the device to which the kernel will be enqueued does not support
   sub-groups i.e. ``kernel->device->info->max_sub_group_count == 0``,
   ``mux_error_feature_unsupported`` **must** be returned.

If an error code other than ``mux_success`` is returned, the values at
``out_local_size_x``, ``out_local_size_y`` and ``out_local_size_z`` **must
not** be considered valid.

.. rubric:: Valid Usage

-  Calls to ``muxQueryLocalSizeForSubGroupCount()`` **shall** be considered
   thread-safe.

muxQueryWFVInfoForLocalSize
~~~~~~~~~~~~~~~~~~~~~~~~~~~

``muxQueryWFVInfoForLocalSize`` queries a ``mux_kernel_t`` for the whole
function vectorization status and dimension work widths that would result from
an enqueue of the kernel with the given local size.

Entry point is optional and **must** return ``mux_error_feature_unsupported``
if it is not supported.

.. code:: c

   mux_result_t muxQueryWFVInfoForLocalSize(mux_kernel_t kernel,
                                            size_t local_size_x,
                                            size_t local_size_y,
                                            size_t local_size_z,
                                            mux_wfv_status_e *out_wfv_status,
                                            size_t *out_work_width_x,
                                            size_t *out_work_width_y,
                                            size_t *out_work_width_z);

-  ``kernel`` - the kernel to query the WFV info for.
-  ``local_size_x`` - the local size in the x dimension of the potential
   enqueue.
-  ``local_size_y`` - the local size in the y dimension of the potential
   enqueue.
-  ``local_size_z`` - the local size in the z dimension of the potential
   enqueue.
-  ``out_wfv_status`` - the whole function vectorization status that would
   result from a kernel enqueue with the specified local size (one of the
   values in the enum `mux_wfv_status_e`_).
-  ``out_work_width_x`` - the work width in the x dimension that would result
   from a kernel enqueue with the specified local size.
-  ``out_work_width_y`` - the work width in the y dimension that would result
   from a kernel enqueue with the specified local size.
-  ``out_work_width_z`` - the work width in the z dimension that would result
   from a kernel enqueue with the specified local size.

.. rubric:: Return Codes

- If ``kernel`` is NULL, ``mux_error_invalid_value`` **must** be returned.
- If ``local_size_x`` is 0 ``mux_error_invalid_value`` **must** be returned.
- If ``local_size_y`` is 0 ``mux_error_invalid_value`` **must** be returned.
- If ``local_size_y`` is 0 ``mux_error_invalid_value`` **must** be returned.
- If ``out_wfv_status`` is NULL and any of ``out_work_width_x``,
  ``out_work_width_y``, and ``out_work_width_z`` are NULL,
  ``mux_error_null_out_parameter`` **must** be returned.
- If the device to which the kernel will be enqueued does not support whole
  function vectorization ``mux_error_feature_unsupported`` **must** be
  returned.

If an error code other than ``mux_success`` is returned, the values at
``out_wfv_status``, ``out_work_width_x``, ``out_work_width_y``, and
``out_work_width_z`` **must not** be considered valid.

.. rubric:: Valid Usage

-  Calls to ``muxQueryWFVInfoForLocalSize()`` **shall** be considered
   thread-safe.
-  Enqueueing ``kernel`` with the specified local size **shall** result in the
   number of work-items executing for each dimension in one invocation of the
   kernel being equal to the dimension work widths returned in
   ``out_work_width_x``, ``out_work_width_y``, and ``out_work_width_z``.

mux_wfv_status_e
''''''''''''''''

.. code:: c

   typedef enum mux_wfv_status_e {
     mux_wfv_status_none,
     mux_wfv_status_error,
     mux_wfv_status_success
   } mux_wfv_status_e;

-  ``mux_wfv_status_none`` - whole function vectorization has not been
   performed on the specified kernel.
-  ``mux_wfv_status_error`` - an error was encountered while trying to perform
   perform whole function vectorization on the specified kernel.
-  ``mux_wfv_status_success`` - whole function vectorization has been
   successfully performed on the specified kernel.

muxDestroyKernel
~~~~~~~~~~~~~~~~

``muxDestroyKernel()`` destroys a previously created ``mux_kernel_t``.

.. code:: c

   void muxDestroyKernel(
       mux_device_t device,
       mux_kernel_t kernel,
       mux_allocator_info_t allocator_info);

-  ``device`` - the device used to create the kernel.
-  ``kernel`` - a kernel previously created by a call to
   ``muxCreateKernel()``.
-  ``allocator_info`` - the user provided allocator ``kernel`` was
   created with.

.. rubric:: Valid Usage

-  Calls to ``muxDestroyKernel()`` with a given ``kernel`` **shall** be
   considered thread-safe.
-  The ``allocator_info`` passed to ``muxDestroyKernel()`` **must** be
   the same ``mux_allocator_info_t`` as was used in the call to
   ``muxCreateKernel()`` that created ``kernel``.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``kernel`` **must** be a valid ``mux_kernel_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

.. rubric:: Validation Rules

-  The ``mux_executable_t`` used in the creation of a ``mux_kernel_t``
   **must not** be destroyed before the kernel has been destroyed.
-  The ``mux_device_t``, that was used in the creation of a
   ``mux_kernel_t`` **must not** be destroyed before the kernel has been
   destroyed.
-  The ``preferred_local_size_x`` field of ``mux_kernel_t`` **must** be
   at least 1.
-  The ``preferred_local_size_y`` field of ``mux_kernel_t`` **must** be
   at least 1.
-  The ``preferred_local_size_z`` field of ``mux_kernel_t`` **must** be
   at least 1.
-  The ``preferred_local_size_x`` field of ``mux_kernel_t`` **must not**
   exceed the value of the ``max_work_group_size_x`` field of the
   ``mux_device_t`` that was used to create the ``mux_kernel_t``.
-  The ``preferred_local_size_y`` field of ``mux_kernel_t`` **must not**
   exceed the value of the ``max_work_group_size_y`` field of the
   ``mux_device_t`` that was used to create the ``mux_kernel_t``.
-  The ``preferred_local_size_z`` field of ``mux_kernel_t`` **must not**
   exceed the value of the ``max_work_group_size_z`` field of the
   ``mux_device_t`` that was used to create the ``mux_kernel_t``.

Memory
------

Memory in Mux represents physical memory allocations that are accessible
on a device.

.. code:: c

   struct mux_memory_s {
     uint64_t size;
     uint32_t properties;
     uint64_t handle;
   };

-  ``size`` - the size (in bytes) that this memory has allocated to it.
-  ``properties`` - the memory properties of the allocated device
   memory.
-  ``handle`` - a handle to the allocated memory. For memory objects
   with the ``mux_memory_property_host_visible`` property this **must**
   be a host accessible pointer.

muxAllocateMemory
~~~~~~~~~~~~~~~~~

``muxAllocateMemory()`` allocates some physical device memory on the
given Mux device.

.. code:: c

   mux_result_t muxAllocateMemory(
       mux_device_t device,
       size_t size,
       uint32_t heap,
       uint32_t memory_properties,
       mux_allocation_type_e allocation_type,
       uint32_t alignment,
       mux_allocator_info_t allocator_info,
       mux_memory_t* out_memory);

-  ``device`` - the ``mux_device_t`` to allocate this memory for.
-  ``size`` - the size in bytes of memory to allocate.
-  ``heap`` - value of a single set bit in the
   ``mux_memory_requirements_s::supported_heaps`` bitfield of the buffer
   or image the device memory is being allocated for.
-  ``memory_properties`` - bitfield of memory properties this allocation
   should support:

   -  ``mux_memory_property_device_local`` - Memory is visible only on
      the device and can not be mapped. Must not be used in combination
      with ``mux_memory_property_host_visible``,
      ``mux_memory_property_host_coherent``, or
      ``mux_memory_property_host_cached``.
   -  ``mux_memory_property_host_visible`` - Memory is visible on host
      and device, can be mapped using ``muxMapMemory()``. Can be used
      with ``mux_memory_property_host_coherent`` or
      ``mux_memory_property_host_cached``.
   -  ``mux_memory_property_host_coherent`` - Memory caches are coherent
      between host and device. No explicit synchronization is required.
      Can be used with ``mux_memory_property_host_visible``, is mutually
      exclusive with ``mux_memory_property_host_cached``.
   -  ``mux_memory_property_host_cached`` - Memory caches are not
      coherent between host and device. Explicit synchronization is
      required, using ``muxMapMemory()`` and ``muxUnmapMemory()``,
      before updates are visible on host or device. Can be used with
      ``mux_memory_property_host_visible``, is mutually exclusive with
      ``mux_memory_property_host_coherent``.

-  ``allocation_type`` - the type of allocation.

   -  ``mux_allocation_type_alloc_host`` - Allocate pinned memory
      visible on both host and device. Used in combination with
      ``mux_memory_property_host_coherent`` or
      ``mux_memory_property_host_cached``.
   -  ``mux_allocation_type_alloc_device`` - Allocate device memory. Can
      be used with ``mux_memory_property_device_local`` or
      ``mux_memory_property_host_visible``.

-  ``alignment`` - minimum alignment in bytes to allocate to, may be
   ``0`` to indicate no alignment requirement.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_memory`` - the created memory, or uninitialized if an error
   occurred.

..

   Note that ``allocator_info`` **must** be used for any allocations
   required for the host-side implementation of this API entry point.
   There is no requirement for it to be used for the allocation of
   physical device memory.

.. rubric:: Return Codes

-  If ``size`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``heap`` is 0 ``mux_error_invalid_value`` **must** be
   returned.
-  If ``memory_properties`` is 0, ``mux_error_invalid_value``
   **must** be returned.
-  If ``alignment`` is not ``0`` and not a power of two
   ``mux_error_invalid_value`` **must** be returned.
-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_memory`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_memory`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxAllocateMemory()`` **shall** be considered thread-safe.
-  The value of ``size`` **must not** exceed the ``allocation_size``
   member of ``device``.
-  If ``heap`` is 1 a successful allocation **must** occur, assuming
   other valid usage conditions are met.
-  ``memory_properties`` **must not** contain any set bits that are not
   specified in ``mux_memory_property_e``.
-  ``allocation_type`` **must** be one of the allocation types specified
   in ``mux_allocation_type_e``.
-  If the ``allocation_capabilities`` member of ``device`` does not have
   the ``mux_allocation_capabilities_coherent_host`` bit set,
   ``allocation_type`` **must not** be
   ``mux_allocation_type_alloc_host``.
-  If the ``allocation_capabilities`` member of ``device`` does not have
   the ``mux_allocation_capabilities_alloc_device`` bit set,
   ``allocation_type`` **must not** be
   ``mux_allocation_type_alloc_device``.

muxCreateMemoryFromHost
~~~~~~~~~~~~~~~~~~~~~~~

``muxCreateMemoryFromHost()`` assigns Mux device visible memory from
pre-allocated host side memory. The device visible memory assigned is
cache coherent with the host allocation, therefore the ``properties``
member of the returned ``mux_memory_t`` object **must** be set to
``mux_memory_property_host_visible`` and
``mux_memory_property_host_coherent``.

``muxFreeMemory()`` should be used to release the created
``mux_memory_t`` object at the end of its lifetime, but will not
deallocate the underlying host memory.

Mapping the created ``mux_memory_t`` object with ``muxMapMemory()``
**must** return a pointer in ``out_data`` based on the ``host_pointer``
argument used to create the memory object.

Entry point is optional as it requires device support for
``mux_allocation_capabilities_cached_host`` and **must** return
``mux_error_feature_unsupported`` if this is not the case.

.. code:: c

   mux_result_t muxCreateMemoryFromHost(
       mux_device_t device,
       size_t size,
       void* host_pointer,
       mux_allocator_info_t allocator_info,
       mux_memory_t* out_memory);

-  ``device`` - the ``mux_device_t`` to create this memory for.
-  ``size`` - the size in bytes of allocated memory.
-  ``host_pointer`` - pointer to pre-allocated host addressable memory,
   **must not** be NULL.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_memory`` - the created memory, or uninitialized if an error
   occurred.

..

   Note that ``allocator_info`` **must** be used for any allocations
   required for the host-side implementation of this API entry point.
   There is no requirement for it to be used for the allocation of
   physical device memory.

.. rubric:: Return Codes

-  If ``device`` does not report
   ``mux_allocation_capabilities_cached_host`` in
   ``mux_device_info_s::allocation_capabilities``,
   ``mux_error_feature_unsupported`` must be returned.
-  If ``size`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``memory_properties`` is 0, ``mux_error_invalid_value``
   **must** be returned.
-  If ``host_pointer`` is NULL, ``mux_error_invalid_value``
   **must** be returned.
-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_memory`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_memory`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxCreateMemoryFromHost()`` **shall** be considered
   thread-safe.
-  The value of ``size`` **must not** exceed the ``allocation_size``
   member of ``device``.

muxFreeMemory
~~~~~~~~~~~~~

``muxFreeMemory()`` frees a previously allocated memory object from a
given Mux device. If ``memory`` was created by a call to
``muxAllocateMemory()`` then the underlying memory **may** be
deallocated. Otherwise, if ``memory`` was created by a call to
``muxCreateMemoryFromHost()`` then the underlying host memory **must
not** be deallocated.

.. code:: c

   void muxFreeMemory(
       mux_device_t device,
       mux_memory_t memory,
       mux_allocator_info_t allocator_info);

-  ``device`` - the ``mux_device_t`` to free memory for.
-  ``memory`` - a memory object previously created by a call to
   ``muxAllocateMemory()`` or ``muxCreateMemoryFromHost()``.
-  ``allocator_info`` - the user provided allocator ``memory`` was
   created with.

.. rubric:: Valid Usage

-  Calls to ``muxFreeMemory()`` with a given ``memory`` **shall** be
   considered thread-safe.
-  The ``device`` passed to ``muxFreeMemory()`` **must** be the same
   ``mux_device_t`` as was used in the call to ``muxAllocateMemory()``
   or ``muxCreateMemoryFromHost()`` that created ``memory``.
-  The ``allocator_info`` passed to ``muxFreeMemory()`` **must** be the
   same ``mux_allocator_info_t`` as was used in the call to
   ``muxAllocateMemory()`` or ``muxCreateMemoryFromHost()`` that created
   ``memory``.
-  ``muxFreeMemory()`` **must not** be called on a ``mux_memory_t`` that
   is currently mapped via ``muxMapMemory()``.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``memory`` **must** be a valid ``mux_memory_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

muxMapMemory
~~~~~~~~~~~~

``muxMapMemory()`` maps a ``mux_memory_t`` to a host accessible address.
Use ``muxFlushMappedMemoryToDevice()`` and
``muxFlushMappedMemoryFromDevice()`` to explicitly synchronize mapped
memory.

.. code:: c

   mux_result_t muxMapMemory(
       mux_device_t device,
       mux_memory_t memory,
       uint64_t offset,
       uint64_t size,
       void** out_data);

-  ``device`` - the ``mux_device_t`` to map memory with.
-  ``memory`` - the ``mux_memory_t`` to map.
-  ``offset`` - the offset (in bytes) into the ``memory`` to map.
-  ``size`` - the size (in bytes) of the ``memory`` to map.
-  ``out_data`` - the host accessible address.

.. rubric:: Return Codes

-  If ``offset`` is greater than the size of ``memory``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``size`` is greater than the size of ``memory``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``offset`` plus ``size`` is greater than the size of ``memory``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``size`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``flags`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``out_data`` is NULL, ``mux_error_null_out_parameter`` **must** be
   returned.
-  If ``mux_memory_property_device_local`` is set in
   ``memory->properties``, ``mux_error_invalid_value`` **must** be
   returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_data`` **must not** be used, and the ``mux_memory_t`` **shall
not** be mapped.

.. rubric:: Valid Usage

-  Calls to ``muxMapMemory()`` **shall** be considered thread-safe.
-  ``memory`` **must** not already be an object mapped with
   ``muxMapMemory``.
-  ``memory`` **must** have property
   ``mux_memory_property_host_visible`` set.

muxFlushMappedMemoryToDevice
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Explicitly update device memory with data residing in host memory.

``muxFlushMappedMemoryToDevice`` is intended to be used with
``mux_memory_t``\ ’s allocated with the
``mux_memory_property_host_cached`` flag set. It updates device memory
with the content currently residing in host memory.

.. code:: c

   mux_result_t muxFlushMappedMemoryToDevice(
       mux_device_t device,
       mux_memory_t memory,
       uint64_t offset,
       uint64_t size);

-  ``device`` - The device where the memory is allocated.
-  ``memory`` - The device memory to be flushed.
-  ``offset`` - The offset in bytes into the device memory to begin the
   range.
-  ``size`` - The size in bytes of the range to flush.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** but returned when;

   -  ``device`` is not a valid ``mux_device_t``.
   -  ``memory`` is not a valid ``mux_memory_t``.
   -  ``offset`` combined with ``size`` is greater than ``memory`` size.

-  Otherwise ``mux_success`` **shall** be returned.

.. rubric:: Valid Usage

-  ``memory`` **must** be previously mapped using ``muxMapMemory()`` or
   have been created using ``muxCreateMemoryFromHost()``.
-  Flushing ``memory`` with the ``mux_memory_property_host_coherent``
   flag **must** return ``mux_success``, as cache coherency makes
   flushing a nop.

muxFlushMappedMemoryFromDevice
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Explicitly update host memory with data residing in device memory.

``muxFlushMappedMemoryFromDevice`` is intended to be used with
``mux_memory_t``\ ’s allocated with the
``mux_memory_property_host_cached`` flag set. It updates host memory
with the content currently residing in device memory.

.. code:: c

   mux_result_t muxFlushMappedMemoryFromDevice(
       mux_device_t device,
       mux_memory_t memory,
       uint64_t offset,
       uint64_t size);

-  ``device`` - The device where the memory is allocated.
-  ``memory`` - The device memory to be flushed.
-  ``offset`` - The offset in bytes into the device memory to begin the
   range.
-  ``size`` - The size in bytes of the range to flush.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when;

   -  ``device`` is not a valid ``mux_device_t``.
   -  ``memory`` is not a valid ``mux_memory_t``.
   -  ``offset`` combined with ``size`` is greater than ``memory`` size.

-  Otherwise ``mux_success`` **shall** be returned.

.. rubric:: Valid Usage

-  ``memory`` **must** be previously mapped using ``muxMapMemory()`` or
   have been created using ``muxCreateMemoryFromHost()``.

muxUnmapMemory
~~~~~~~~~~~~~~

``muxUnmapMemory()`` invalidates the host pointer returned from
``muxMapMemory()`` meaning ``mux_memory_t`` is no longer addressable
from the host.

.. code:: c

   mux_result_t muxUnmapMemory(
       mux_device_t device,
       mux_memory_t memory);

-  ``device`` - the ``mux_device_t`` to unmap memory with.
-  ``memory`` - the ``mux_memory_t`` to unmap.

.. rubric:: Return Codes

-  ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxUnmapMemory()`` **shall** be considered thread-safe.

.. rubric:: Validation Rules

-  The ``mux_device_t``, that was used in the creation of the
   ``mux_memory_t``, **must not** be destroyed before the memory has
   been destroyed.

Buffers
-------

Buffers in Mux are a C-style array on a device.

muxCreateBuffer
~~~~~~~~~~~~~~~

``muxCreateBuffer()`` creates a buffer from a given Mux device.

.. code:: c

   mux_result_t muxCreateBuffer(
       mux_device_t device,
       size_t size,
       mux_allocator_info_t allocator_info,
       mux_buffer_t* out_buffer);

-  ``device`` - the ``mux_device_t`` to create this buffer with.
-  ``size`` - the size (in bytes) of this buffer.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_buffer`` - the newly created buffer.

..

   Note that ``allocator_info`` **must** be used for any allocations
   required for the host-side implementation of this API entry point.
   There is no requirement for it to be used for the allocation of
   physical device memory.

.. rubric:: Return Codes

-  If ``size`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_buffer`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_buffer`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxCreateBuffer()`` **shall** be considered thread-safe.

muxDestroyBuffer
~~~~~~~~~~~~~~~~

``muxDestroyBuffer()`` destroys a buffer using a given Mux device.

.. code:: c

   void muxDestroyBuffer(
       mux_device_t device,
       mux_buffer_t buffer,
       mux_allocator_info_t allocator_info);

-  ``device`` - the ``mux_device_t`` to destroy this buffer with.
-  ``buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()``.
-  ``allocator_info`` - the user provided allocator ``buffer`` was
   created with.

.. rubric:: Valid Usage

-  Calls to ``muxDestroyBuffer()`` with a given ``buffer`` **shall** be
   considered thread-safe.
-  The ``device`` passed to ``muxDestroyBuffer()`` **must** be the same
   ``mux_device_t`` as was used in the call to ``muxCreateBuffer()``
   that created ``buffer``.
-  The ``allocator_info`` passed to ``muxDestroyBuffer()`` **must** be
   the same ``mux_allocator_info_t`` as was used in the call to
   ``muxCreateBuffer()`` that created ``buffer``.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``buffer`` **must** be a valid ``mux_buffer_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

muxBindBufferMemory
~~~~~~~~~~~~~~~~~~~

``muxBindBufferMemory()`` binds a ``mux_buffer_t`` to a ``mux_memory_t``
memory location using a given Mux device.

.. code:: c

   mux_result_t muxBindBufferMemory(
       mux_device_t device,
       mux_memory_t memory,
       mux_buffer_t buffer,
       uint64_t offset);

-  ``device`` - the ``mux_device_t`` to bind the memory and buffer with.
-  ``memory`` - a memory previously created by a call to
   ``muxAllocateMemory()``.
-  ``buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()``.
-  ``offset`` - the offset (in bytes) into ``memory`` to bind ``buffer``
   to.

.. rubric:: Return Codes

-  If ``offset`` would specify a byte offset greater than the size of
   ``memory``, ``mux_error_invalid_value`` **must** be returned.
-  If ``buffer``\ ’s size is greater than the size of ``memory``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``buffer``\ ’s size plus ``offset`` is greater than the size of
   ``memory``, ``mux_error_invalid_value`` **must** be returned.
-  If ``memory`` is not a valid ``mux_memory_t`` instance,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``buffer`` **must** be re-bound to a ``mux_memory_t`` successfully
before being used.

.. rubric:: Valid Usage

-  Calls to ``muxBindBufferMemory()`` for a given ``memory`` and a given
   ``buffer``, **shall** be considered thread-safe.
-  ``mux_buffer_t``\ ’s are allowed to alias the same memory, but **must
   not** be being executed upon within a ``mux_queue_t`` simultaneously.
-  ``mux_buffer_t`` **may** be re-bound (via a call to
   ``muxBindBufferMemory()``) any number of times.
-  A ``mux_memory_t`` that is currently being bound to a
   ``mux_buffer_t``, **must not** be destroyed before the buffer has
   been destroyed.

.. rubric:: Validation Rules

-  The ``mux_device_t``, that was used in the creation of the
   ``mux_buffer_t``, **must not** be destroyed before the buffer has
   been destroyed.

Images
------

Images are represented by the ``mux_image_t`` handle which contains
metadata about images. ``mux_image_t`` objects do not hold ownership of
physical device memory, this delegated to ``mux_memory_t`` objects.
Images are acted upon by a number of entry points as defined below.

A Mux implementation **must** populate the following attributes of the
``mux_image_s`` structure so that the application can use this
information to correctly use ``mux_image_t`` objects.

.. code:: c

   struct mux_image_s {
     mux_memory_requirements_s memory_requirements;
     mux_image_type_e type;
     mux_image_format_e format;
     uint32_t pixel_size;
     mux_extent_3d_t size;
     uint32_t array_layers;
     uint64_t row_size;
     uint64_t slice_size;
     mux_image_tiling_e tiling;
   };

-  ``memory_requirements`` - the device memory requirements for the
   image.
-  ``type`` - the type of this image.
-  ``format`` - the pixel format of this image.
-  ``pixel_size`` - the size in bytes of a single pixel.
-  ``size`` - the width, height, depth of this image.
-  ``array_layers`` - the number of array layers in the image.
-  ``row_size`` - the size in bytes of an image row.
-  ``slice_size`` - the size in bytes of an image slice.
-  ``tiling`` - the current image tiling mode of this image.

muxCreateImage
~~~~~~~~~~~~~~

The ``muxCreateImage()`` function is used to create a ``mux_image_t``
object, it describes the images type and dimensions. This does not
allocate physical device memory to store the image data, to do this
``muxAllocateMemory()`` must be used, the ``mux_memory_t`` can then be
bound to the ``mux_image_t`` using ``muxBindImageMemory()``.

.. code:: c

   mux_result_t muxCreateImage(
       mux_device_t device,
       mux_image_type_e type,
       mux_image_format_e format,
       uint32_t width,
       uint32_t height,
       uint32_t depth,
       uint32_t array_layers,
       uint64_t row_size,
       uint64_t slice_size,
       mux_allocator_info_t allocator_info,
       mux_image_t* out_image);

-  ``device`` - The device where the image will be used.
-  ``type`` - The type of image to create, one of ``mux_image_type_e``.
-  ``format`` - The pixel data format of the image to create, one of
   ``mux_image_format_e``.
-  ``width`` - The width of the image in pixels, must be greater than
   one and less than max width.
-  ``height`` - The height of the image in pixels, must be zero for 1d
   images, must be greater than one and less than max height for 2d and
   3d images.
-  ``depth`` - The depth of the image in pixels, must be zero for 1d and
   2d images, must be greater than one and less than max depth for 3d
   images.
-  ``array_layers`` - The number of layers in an image array, must be
   ``0`` for non image arrays, must be less than max array layers for
   image arrays.
-  ``row_size`` - The size of an image row in bytes.
-  ``slice_size`` - The size on an image slice in bytes.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_image`` - The created image, or uninitialized if an error
   occurred.

..

   Note that ``allocator_info`` **must** be used for any allocations
   required for the host-side implementation of this API entry point.
   There is no requirement for it to be used for the allocation of
   physical device memory.

.. rubric:: Return Codes

-  If ``device`` is ``NULL``, ``mux_error_invalid_value``
   **shall** be returned.
-  ``mux_error_invalid_value`` **shall** be returned if any of the
   following conditions are met

   -  For ``image_type`` of ``mux_image_type_1d``

      -  If ``width`` is equal to ``0`` or is greater than
         ``max_image_dimension_1d``.
      -  If ``height`` is not equal to ``1``.
      -  If ``depth`` is not equal to ``1``.

   -  For ``image_type`` of ``mux_image_type_2d``

      -  If ``width`` is equal to ``0`` or is greater than
         ``max_image_dimension_2d``.
      -  If ``height`` is equal to ``0`` or is greater than
         ``max_image_dimension_2d``.
      -  If ``depth`` is not equal to ``1``.

   -  For ``image_type`` of ``mux_image_type_3d``

      -  If ``width`` is equal to ``0`` or is greater than
         ``max_image_dimension_3d``.
      -  If ``height`` is equal to ``0`` or is greater than
         ``max_image_dimension_3d``.
      -  If ``depth`` is equal to ``0`` or is greater than
         ``max_image_dimension_3d``.

   -  If ``array_layers`` is greater than
      ``mux_device_s::max_image_array_layers``.
   -  If ``out_image`` is ``NULL``, ``mux_error_null_out_parameter``
      **shall** be returned.
   -  If an allocation failed ``mux_error_out_of_memory`` **shall** be
      returned.

-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  Otherwise ``mux_success`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCreateImage()`` for a given ``device``, **shall** be
   considered thread-safe.
-  ``mux_image_t``\ ’s are allowed to alias the same memory, but the
   aliased ``mux_memory_t`` **must not** be accessed simultaneously.
-  A ``mux_memory_t`` that is currently bound to a ``mux_buffer_t``
   **must not** be destroyed before the ``mux_image_t`` has been
   destroyed.

muxDestroyImage
~~~~~~~~~~~~~~~

Destroy a ``mux_image_t`` object using ``muxDestroyImage()``, destroying
an image unbinds a bound ``mux_memory_t``.

.. code:: c

   void muxDestroyImage(
       mux_device_t device,
       mux_image_t image,
       mux_allocator_info_t allocator_info);

-  ``device`` - the ``mux_device_t`` to destroy this image with.
-  ``image`` - an image previously created by a call to
   ``muxCreateImage()``.
-  ``allocator_info`` - the user provided allocator ``image`` was
   created with.

.. rubric:: Valid Usage

-  Calls to ``muxDestroyImage()``, for a given ``mux_device_t``,
   **shall** be considered thread-safe.
-  The ``device`` instance used for ``muxCreateImage()`` **must** be the
   same ``device`` passed to ``muxDestroyImage()``.
-  The ``allocator_info`` passed to ``muxDestroyImage()`` **must** be
   the same ``mux_allocator_info_t`` as was used in the call to
   ``muxCreateImage()`` that created ``image``.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``image`` **must** be a valid ``mux_image_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

muxBindImageMemory
~~~~~~~~~~~~~~~~~~

To provide physical device memory backing to a ``mux_image_t`` object,
the ``muxBindImageMemory()`` function is provided. This is used to
specify the region of the ``mux_memory_t`` that should be used to store
the ``mux_image_t``\ ’s data on the device.

.. code:: c

   mux_result_t muxBindImageMemory(
       mux_device_t device,
       mux_memory_t memory,
       mux_image_t image,
       uint64_t offset);

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when any of
   the
-  following conditions are met

   -  ``device`` is not a valid ``mux_device_t`` instance.
   -  ``memory`` is not a valid ``mux_memory_t`` instance.
   -  ``image`` is not a valid ``mux_image_t`` instance.
   -  ``offset + image->memory_requirements.size`` is greater than
      ``memory->size``.
   -  ``offset`` is not a multiple of
      ``image->memory_requirements.alignment``.

-  Otherwise ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxBindImageMemory()`` for the given ``device`` and
   ``memory``, **shall** be considered thread-safe.
-  ``mux_image_t``\ ’s **may** alias the same memory, but the aliased
   ``mux_memory_t`` **must not** be executed simultaneously.
-  ``mux_image_t``\ ’s **may** be re-bound (via a call to
   ``muxBindImageMemory()``) any number of times.
-  A ``mux_memory_t`` that is currently bound to a ``mux_image_t``
   **must not** be destroyed before the image has been destroyed.

muxGetSupportedImageFormats
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Query the Mux device for a list of supported image formats.

.. code:: c

   mux_result_t muxGetSupportedImageFormats(
       mux_device_t device,
       mux_image_type_e image_type,
       mux_allocation_type_e allocation_type,
       uint32_t count,
       mux_image_format_e* out_formats,
       uint32_t* out_count);

-  ``device`` - The device to query for supported image formats.
-  ``image_type`` - The type of the image.
-  ``allocation_type`` - The required allocation capabilities of the
   image.
-  ``count`` - The element count of the ``out_formats`` array, must be
   greater than zero if ``out_formats`` is not null, and zero otherwise.
-  ``out_formats`` - Return the list of supported formats, may be null.
   Storage must be an array of ``out_count`` elements.
-  ``out_count`` - Return the number of supported formats, may be null.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when any of
   the following conditions are met

   -  ``device`` is not a valid ``mux_device_t`` instance.
   -  ``image_type`` is not a valid ``mux_image_type_e``.
   -  ``allocation_type`` is not a valid ``mux_allocation_type_e``.
   -  ``count`` is ``0`` and ``out_formats`` is not ``NULL``.

-  Otherwise ``mux_success`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxGetSupportedImageFormats()`` for a given ``device``
   **shall** be considered thread-safe.
-  When ``out_formats`` is ``NULL``, ``count`` **must** be ``0``.
-  When ``out_formats`` is not ``NULL``, ``count`` **must not** be
   ``0``.
-  ``out_count`` **may** be ``NULL``.

Queries
-------

Queries allow the application to query the implementation about runtime
behavior by providing a mechanism to report performance data. Multiple
queries **may** be active at one time, a query is considered to be
active as long as there is an existing query pool to which its
corresponding ``mux_query_type_e`` (and its specific ``uuid`` in the
case of ``mux_query_type_counter``) was passed during creation. Note
that in the case of counter queries, each active query takes a number
of hardware counters to support, as denoted by
``mux_query_counter_s::hardware_counters``. The number of hardware
counters needed to support all active counter queries **must** not
exceed the device's limit, as defined by
``mux_device_info_s::max_active_hardware_counters``. The storage for
the query data resides in a ``mux_query_pool_t`` object.

.. code:: c

   struct mux_query_pool_s {
     mux_id_t id;
     mux_query_type_e type;
     uint32_t count;
   };

-  ``id`` - the ID of this query pool object.
-  ``type`` - the type of the query pool (one of the values in the enum
   ``mux_query_type_e``).
-  ``count`` - the total number of queries that can be stored in the
   query pool.

muxCreateQueryPool
~~~~~~~~~~~~~~~~~~

Create a query pool into which data can be stored about the runtime
behavior of commands on a queue.

.. code:: c

   mux_result_t muxCreateQueryPool(
       mux_queue_t queue,
       mux_query_type_e query_type,
       uint32_t query_count,
       const mux_query_counter_config_t* query_counter_configs,
       mux_allocator_info_t allocator_info,
       mux_query_pool_t* out_query_pool);

-  ``queue`` - the Mux queue to create the query pool with.
-  ``query_type`` - the type of query pool to create (one of the values
   in the enum `mux_query_type_e`_).
-  ``query_count`` - the number of queries to allocate storage for.
-  ``query_counter_configs`` - array of query counter configuration data
   with length ``query_count``, **must** be provided when ``query_type``
   is ``mux_query_type_counter``, see `mux_query_counter_config_s`_.
-  ``allocator_info`` - allocator information.
-  ``out_query_pool`` - the newly created query pool, or uninitialized
   if an error occurred.

.. rubric:: Return Codes

-  If ``queue`` is not a valid ``mux_queue_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_type`` is not a value defined in ``mux_query_type_e``,
   ``mux_error_invalid_value`` **should** be returned.
-  If ``query_count`` is 0, ``mux_error_invalid_value`` **must**
   be returned.
-  If ``out_query_pool`` is NULL, ``mux_error_null_out_parameter``
   **must** be returned.
-  If an allocation failed ``mux_error_out_of_memory`` **shall** be
   returned.
-  If the number of hardware counters needed to accomodate the query
   counters in ``query_counter_configs`` would bring the number needed
   to support all active counters above the maximum supported by the
   device, ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

..

   Note: ``query_type`` validation **should** be performed by the
   target.

.. rubric:: Valid Usage

-  Calls to ``muxCreateQueryPool`` **shall** be considered thread-safe.

mux_query_type_e
''''''''''''''''

.. code:: c

   typedef enum mux_query_type_e {
     mux_query_type_duration
     mux_query_type_counter
   } mux_query_type_e;

-  ``mux_query_type_duration`` - query the command duration with
   ``start`` and ``end`` timestamps, results are stored in an array of
   ``mux_query_duration_result_s``, the ``start`` and ``end`` timestamps
   **must** be CPU timestamps which **may** require interpolation of
   device timestamps, only a single command duration query **shall** be
   enabled in a command buffer at one time.
-  ``mux_query_type_counter`` - query the values of an enabled set of
   hardware counters, results are stored in an array of
   ``mux_query_counter_result_s``, the union member which contains the
   result is defined by the ``mux_query_counter_s`` struct’s ``storage``
   member with a matching ``uuid`` to the enabled counter.

mux_query_counter_config_s
''''''''''''''''''''''''''

.. code:: c

   struct mux_query_counter_config_s {
     uint32_t uuid;
     void* data;
   };

-  ``uuid`` - the unique ID of the query counter, see
   `muxGetSupportedQueryCounters <#muxgetsupportedquerycounters>`__.
-  ``data`` - data used to specify how a counter is to be configured,
   **may** be NULL.

muxDestroyQueryPool
~~~~~~~~~~~~~~~~~~~

Destroy a query pool previously created with ``muxCreateQueryPool``.

.. code:: c

   void muxDestroyQueryPool(
       mux_queue_t queue,
       mux_query_pool_t query_pool,
       mux_allocator_info_t allocator_info);

-  ``queue`` - the Mux queue the query pool was created with.
-  ``query_pool`` - the Mux query pool to destroy.
-  ``allocator_info`` - allocator information.

.. rubric:: Valid Usage

-  Calls to ``muxDestroyQueryPool()`` **shall** be considered
   thread-safe.
-  The ``queue`` passed to ``muxDestroyQueryPool()`` **must** be the
   same ``mux_queue_t`` as was used in the call to
   ``muxCreateQueryPool()`` that created ``query_pool``.
-  The ``allocator_info`` passed to ``muxDestroyQueryPool`` **must** be
   the same ``mux_allocator_info_t`` as was used in the call to
   ``muxCreateQueryPool()`` that created ``query_pool``.
-  ``queue`` **must** be a valid ``mux_queue_t``.
-  ``query_pool`` **must** be a valid ``mux_query_pool_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

muxGetQueryPoolResults
~~~~~~~~~~~~~~~~~~~~~~

Get query results previously written to the query pool, it copies the
requested query pool results previously written during runtime on the
Mux queue into the user provided storage.

.. code:: c

   mux_result_t muxGetQueryPoolResults(
       mux_queue_t queue,
       mux_query_pool_t query_pool,
       uint32_t query_index,
       uint32_t query_count,
       size_t size,
       void* data,
       size_t stride);

-  ``queue`` - the Mux queue the query pool was created with.
-  ``query_pool`` - the Mux query pool to get the results from.
-  ``query_index`` - the query index of the first query to get the
   result of.
-  ``query_count`` - the number of queries to get the results of.
-  ``size`` - the size in bytes of the memory pointed to by ``data``.
-  ``data`` - a pointer to the memory to write the query results into,
   if ``query_pool->type`` is ``mux_query_type_duration`` then ``data``
   **shall** point to an array of `mux_query_duration_result_s`_, if
   ``query_qool->type`` is ``mux_query_type_counter`` then ``data``
   **shall** point to an array of `mux_query_counter_result_s`_.
-  ``stride`` - the stride in bytes between query results to be written
   into ``data``.

.. rubric:: Return Codes

-  If ``queue`` is not a valid ``mux_queue_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_pool`` is not a valid ``mux_query_pool_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_index`` is greater than or equal to ``query_pool->count``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_count + query_index`` is greater than
   ``query_pool->count``, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``size`` is smaller than the storage required for the requested
   query results, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``data`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``stride`` is smaller than the storage required for a single query
   result, ``mux_error_invalid_value`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxGetQueryPoolResults()`` **shall** be considered
   thread-safe.
-  ``muxGetQueryPoolResults()`` **must not** be called when the
   ``query_pool`` is currently being used in a ``mux_command_buffer_t``
   that has been dispatched but is not complete.
-  Results returned by ``muxGetQueryPoolResults()`` **must** be
   available as soon as a dispatched ``mux_command_buffer_t`` which uses
   the ``query_pool`` has completed.

mux_query_duration_result_s
'''''''''''''''''''''''''''

.. code:: c

   struct mux_query_duration_result_s {
     uint64_t start;
     uint64_t end;
   };

-  ``start`` - the CPU timestamp at the start of the command, in
   nanoseconds.
-  ``end`` - the CPU timestamp at the end of the command, in
   nanoseconds.

mux_query_counter_result_s
''''''''''''''''''''''''''

.. code:: c

   struct mux_query_counter_result_s {
     union {
       int32_t int32;
       int64_t int64;
       uint32_t uint32;
       uint64_t uint64;
       float float32;
       double float64;
     };
   };

-  ``int32`` - contains the result when the associated enabled counter’s
   ``storage`` is ``mux_query_counter_storage_int32``.
-  ``int64`` - contains the result when the associated enabled counter’s
   ``storage`` is ``mux_query_counter_storage_int64``.
-  ``uint32`` - contains the result when the associated enabled
   counter’s ``storage`` is ``mux_query_counter_storage_uint32``.
-  ``uint64`` - contains the result when the associated enabled
   counter’s ``storage`` is ``mux_query_counter_storage_uint64``.
-  ``float32`` - contains the result when the associated enabled
   counter’s ``storage`` is ``mux_query_counter_storage_float32``.
-  ``float64`` - contains the result when the associated enabled
   counter’s ``storage`` is ``mux_query_counter_storage_float64``.

muxGetSupportedQueryCounters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Get the list of supported query counters for a device’s queue type.

.. code:: c

   mux_result_t muxGetSupportedQueryCounters(
       mux_device_t device,
       mux_queue_type_e queue_type,
       uint32_t count,
       mux_query_counter_t* out_counters,
       mux_query_counter_description_t* out_descriptions,
       uint32_t* out_count);

-  ``device`` - a Mux device.
-  ``queue_type`` - the type of queue to get the supported query
   counters list for.
-  ``count`` - the element count of the ``out_counters`` and
   ``out_descriptions`` arrays, **must** be greater than zero if
   ``out_counters`` is not NULL and zero otherwise.
-  ``out_counters`` - return the list of supported query counters,
   **may** be NULL. Storage **must** be an array of ``count`` elements,
   see `mux_query_counter_s`_.
-  ``out_descriptions`` - return the list of descriptions of support
   query counters, **may** be NULL. Storage **must** be an array of
   ``count`` elements, see `mux_query_counter_description_s`_.
-  ``out_count`` - return the total count of supported query counters.

.. rubric:: Return Codes

-  If ``device`` is not a valid ``mux_device_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``device->info->query_counter_support`` is false,
   ``mux_error_feature_unsupported`` **must** be returned.
-  If ``queue_type`` is not a value defined in ``mux_queue_type_e``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``count`` is zero and ``out_counters`` or ``out_descriptions`` are
   not NULL, ``mux_error_invalid_value`` **must** be returned.
-  If ``count`` is not zero and ``out_counters`` and
   ``out_descriptions`` are NULL, ``mux_error_null_out_parameter``
   **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxGetSupportedQueryCounters`` **shall** be considered
   thread-safe.

mux_query_counter_s
'''''''''''''''''''

.. code:: c

   struct mux_query_counter_s {
     mux_query_counter_unit_e unit;
     mux_query_counter_storage_e storage;
     uint32_t uuid;
     uint32_t hardware_counters;
   };

-  ``unit`` - the unit of the query counter result.
-  ``storage`` - the storage type of the query counter result.
-  ``uuid`` - the unique ID of the query counter, this is used to enable
   a counter.
-  ``hardware_counters`` - the number of hardware counters required to
   support this query counter while it's active.

mux_query_counter_description_s
'''''''''''''''''''''''''''''''

.. code:: c

   struct mux_query_counter_description_s {
     char name[256];
     char category[256];
     char description[256];
   };

-  ``name`` - the counter name stored in a UTF-8 encoded null terminated
   array of 256 characters.
-  ``category`` - the counter category stored in a UTF-8 encoded null
   terminated array of 256 characters.
-  ``description`` - the counter description stored in a UTF-8 encoded
   null terminated array of 256 characters.

muxGetQueryCounterRequiredPasses
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Get the number of passes required for valid query counter results.

.. code:: c

   mux_result_t muxGetQueryCounterRequiredPasses(
       mux_queue_t queue,
       uint32_t query_count,
       const mux_query_counter_config_t* query_counter_configs,
       uint32_t* out_pass_count);

-  ``queue`` - a mux queue.
-  ``query_count`` - the number of elements in the array pointed to by
   ``query_counter_configs``.
-  ``query_counter_configs`` - array of query counter configuration data
   with length ``query_count``, **must** be provided when ``query_type``
   is ``mux_query_type_counter``, otherwise **must** be NULL.
-  ``out_pass_count`` - return the number of passes required to produce
   valid results for the given list of query counters.

.. rubric:: Return Codes

-  If ``queue`` is not a valid ``mux_queue_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_count`` is zero, ``mux_error_invalid_value``
   **must** be returned.
-  If ``query_counter_configs`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``out_pass_count`` is NULL, ``mux_error_null_out_parameter``
   **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxGetQueryCounterRequiredPasses`` **shall** be considered
   thread-safe.

Command Buffers
---------------

Command buffers in Mux are a collection of commands that will be
dispatched and executed on a device. Commands within a command buffer
**must** be executed *as if* in the order they are pushed onto the
command buffer.

   Command buffer implementations **must not** access Mux semaphores of
   completed command buffers they wait on, otherwise code using Mux
   **may** deadlock due to resetting semaphores. One approach to
   implement command buffers is for command buffers to track which other
   command buffers wait on them while adding a counter to represent the
   number of command buffers they are waiting on. When a command buffer
   completes then decrease the counters of all command buffers waiting on
   it. Command buffers are only ready to run when their waiting counter
   is zero.

Synchronization of commands within the same command-buffer is done by
``mux_sync_point_s`` synchronization points (sync-point). Each command recording
entry-point can return a sync-point for other commands to wait on, and in turn
take a list of sync-points to wait on itself. There are no create or destroy
entry-points for user control of the sync-point lifetimes. Instead
``mux_sync_point_s`` objects are valid for the lifetime of the command-buffer
they were created from.

.. warning::

  The restriction that commands in a command-buffer must be executed as if
  in-order will be relaxed once sync-points are fully implemented for all
  targets.

muxCreateCommandBuffer
~~~~~~~~~~~~~~~~~~~~~~

``muxCreateCommandBuffer()`` creates a command_buffer from a given Mux
device.

.. code:: c

   mux_result_t muxCreateCommandBuffer(
       mux_device_t device,
       mux_callback_info_t callback_info,
       mux_allocator_info_t allocator_info,
       mux_command_buffer_t* out_command_buffer);

-  ``device`` - the ``mux_device_t`` to create this command buffer with.
-  ``callback_info`` - the user provided callback, which **may** be
   null, can be used by the implementation to provide detailed messages
   about command buffer execution to the user.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_command_buffer`` - the newly created command buffer.

.. rubric:: Return Codes

-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_command_buffer`` is NULL, ``mux_error_null_out_parameter``
   **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_command_buffer`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxCreateCommandBuffer()`` **shall** be considered
   thread-safe.

.. rubric:: Validation Rules

-  The ``mux_device_t`` that was used in the creation of the
   ``mux_command_buffer_t`` **must not** be destroyed before the command
   group has been destroyed.

muxFinalizeCommandBuffer
~~~~~~~~~~~~~~~~~~~~~~~~

``muxFinalizeCommandBuffer()`` finalizes a ``mux_command_buffer_t``.

.. code:: c

   mux_result_t muxFinalizeCommandBuffer(
       mux_command_buffer_t command_buffer);

-  ``command_buffer`` - the ``mux_command_buffer_t`` to be finalized.

.. rubric:: Return Codes

-  If ``command_buffer`` is ``NULL``, ``mux_error_null_out_parameter`` **must**
   be returned.

-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **may** not be in a valid state.

.. rubric:: Valid Usage:

- Calls to ``muxFinalizeCommandBuffer()`` operating on distinct
  ``command_buffer``\ ’s **shall** be considered thread-safe.
- Subsequent calls to ``muxFinalizeCommandBuffer()`` with the same ``command_buffer``
  will have no effect on ``command_buffer``.

.. rubric::  Validation Rules:

- ``command_buffer`` **must** not have further commands pushed to it after
  ``muxFinalizeCommandBuffer()`` is called unless ``muxResetCommandBuffer()``
  is called first.
- All ``mux_command_buffer_t`` s must be finalized before being passed to a
  ``muxDispatch()`` call.

muxCloneCommandBuffer
~~~~~~~~~~~~~~~~~~~~~

``muxCloneCommandBuffer()`` clones a ``mux_command_buffer_t``.

Entry point is optional and **must** return ``mux_error_feature_unsupported`` if
it is not supported.

.. code:: c

   mux_result_t muxCloneCommandBuffer(
       mux_device_t device,
       mux_allocator_info_t allocator_info,
       mux_command_buffer_t command_buffer,
       mux_command_buffer_t* out_command_buffer);

- ``device`` - the ``mux_device_t`` to create the new command buffer with.
- ``allocator_info`` - the user provided allocator **should** be used for host
  memory allocations as described in the `Allocators <#allocators>`__ section.
- ``command_buffer`` - the ``mux_command_buffer_t`` to be cloned.
- ``out_command_buffer`` - the newly created command buffer.

.. note::
  Currently the caller expectations of this entry-point are that the clone
  needs to deep copy the ndrange kernel commands only, and other
  commands and resources may be shallow copied.

.. rubric:: Return Codes:

- If ``device`` does not support cloning command buffers
  i.e. ``mux_device_info_s::can_clone_command_buffers == false``
  ``mux_error_feature_unsupported`` must be returned.
- If ``allocator_info`` function pointer fields are ``NULL``,
  ``mux_error_null_allocator_callback`` **must** be returned.
- If ``command_buffer`` is ``NULL``, ``mux_error_invalid_value`` **must** be
  returned.
- If ``out_command_buffer`` is ``NULL``, ``mux_error_null_out_parameter`` **must** be
  returned.
- Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` or ``mux_error_feature_unsupported``
is returned, ``out_command_buffer`` **may** not be in a valid state.

.. rubric:: Valid Usage:

- Calls to ``muxCloneCommandBuffer()`` operating on distinct ``command_buffers``'s
  **shall** be considered thread-safe.
- ``mux_command_buffer_t``s passed as the ``mux_command_buffer`` parameter **must** be
  in the *finalized* state.
- A ``mux_device_t`` passed as the ``device`` parameter **must** be the same
  ``device`` that was passed to the ``muxCreateCommandBuffer`` call used create the
  ``command_buffer`` parameter.

.. rubric:: Validation Rules:

- ``out_command_buffer`` **must** not have further commands pushed to it after
  ``muxCloneCommandBuffer()`` is called unless ``muxResetCommandBuffer()``
  is called first.
- The ``mux_device_t`` that was used in the creation of the
  ``mux_command_buffer_t`` that is returned via the ``out_command_buffer`` parameter
  **must not** be destroyed before the command buffer has been destroyed.

muxDestroyCommandBuffer
~~~~~~~~~~~~~~~~~~~~~~~

``muxDestroyCommandBuffer()`` destroys a previously created
``mux_command_buffer_t``.

.. code:: c

   void muxDestroyCommandBuffer(
       mux_device_t device,
       mux_command_buffer_t command_buffer,
       mux_allocator_info_t allocator_info);

-  ``device`` - the device to create the command buffer with.
-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``allocator_info`` - the user provided allocator ``command_buffer``
   was created with.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.

.. rubric:: Valid Usage

-  Calls to ``muxDestroyCommandBuffer()`` with a given ``command_buffer``
   **shall** be considered thread-safe.
-  The ``allocator_info`` passed to ``muxDestroyCommandBuffer()``
   **must** be the same ``mux_allocator_info_t`` as was used in the call
   to ``muxCreateCommandBuffer()`` that created ``command_buffer``.
-  ``muxDestroyCommandBuffer()`` **must not** be called in a
   ``muxDispatch()``\ ’s ``user_function`` callback.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``command_buffer`` **must** be a valid ``mux_command_buffer_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

muxResetCommandBuffer
~~~~~~~~~~~~~~~~~~~~~

``muxResetCommandBuffer()`` resets a previously created
``mux_command_buffer_t``.

.. code:: c

   mux_result_t muxResetCommandBuffer(
       mux_command_buffer_t command_buffer);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.

.. rubric:: Return Codes

-  ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxResetCommandBuffer()`` with a given ``command_buffer``
   **shall** be considered thread-safe.
-  ``muxResetCommandBuffer()`` **must not** be called in a
   ``muxDispatch()``\ ’s ``user_function`` callback.

muxCommandReadBuffer
~~~~~~~~~~~~~~~~~~~~

Push a command to a command buffer to read a ``mux_buffer_t`` to a host
pointer.

.. code:: c

   mux_result_t muxCommandReadBuffer(
       mux_command_buffer_t command_buffer,
       mux_buffer_t buffer,
       uint64_t offset,
       void* host_pointer,
       uint64_t size,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be read from.
-  ``offset`` - the offset (in bytes) into ``buffer`` to read.
-  ``host_pointer`` - the host pointer to write to.
-  ``size`` - the size (in bytes) to copy from ``buffer`` to
   ``host_pointer``.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``offset`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``offset`` plus ``size`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``host_pointer`` is NULL, ``mux_error_invalid_value``
   **must** be returned.
-  If ``size`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``size`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandReadBuffer()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` and ``buffer`` passed to
   ``muxCommandReadBuffer()`` **must** have been created using the same
   ``mux_device_t``.
-  The ``buffer`` passed to ``muxCommandReadBuffer()`` **must** be bound to
   a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``buffer`` passed to ``muxCommandReadBuffer()`` **must not** be
   re-bound to a new ``mux_memory_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandReadBufferRegions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Push a command to a command buffer to read regions from a
``mux_buffer_t`` to regions in a host pointer.

.. code:: c

   mux_result_t muxCommandReadBufferRegions(
       mux_command_buffer_t command_buffer,
       mux_buffer_t buffer,
       void* host_pointer,
       mux_buffer_region_info_t* regions,
       uint64_t regions_length,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be read from.
-  ``host_pointer`` - the host pointer to write to.
-  ``regions`` - the regions of the ``buffer`` to write to regions of
   the ``host_pointer``.
-  ``regions_length`` - the number of elements in ``regions``.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``host_pointer`` is NULL, ``mux_error_invalid_value``
   **must** be returned.
-  If ``regions`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If any of the ``regions`` contain a ``region`` that has an
   ``x``,\ ``y``, or ``z`` attribute set to zero
   ``mux_error_invalid_value`` **must** be returned. For a 2D
   region ``z`` should be 1, for a 1D region ``y`` and ``z`` should be
   1.
-  If any of the ``src_desc`` or ``dst_desc`` contain an ``x`` or ``y``
   value that is less than the ``x`` or ``y`` value in ``region``,
   ``mux_error_invalid_value`` **must** be returned.
-  If the ``region`` offset by the ``src_offset`` is out of the bounds
   of the ``buffer``, ``mux_error_invalid_value`` **must** be
   returned.
-  If any of the ``regions`` offset by the ``src_offset`` overlap, then
   ``mux_error_invalid_value`` **must** be returned.
-  If ``regions_length`` is 0, ``mux_error_invalid_value``
   **must** be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandReadBufferRegions()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` and ``buffer`` passed to
   ``muxCommandReadBufferRegions()`` **must** have been created using the
   same ``mux_device_t``.
-  The ``buffer`` passed to ``muxCommandReadBufferRegions()`` **must** be
   bound to a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``buffer`` passed to ``muxCommandReadBufferRegions()`` **must not**
   be re-bound to a new ``mux_memory_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandWriteBuffer
~~~~~~~~~~~~~~~~~~~~~

Push a command to a command buffer to write to a ``mux_buffer_t`` from a
host pointer.

.. code:: c

   mux_result_t muxCommandWriteBuffer(
       mux_command_buffer_t command_buffer,
       mux_buffer_t buffer,
       uint64_t offset,
       const void* host_pointer,
       uint64_t size,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be written to.
-  ``offset`` - the offset (in bytes) into ``buffer`` to write.
-  ``host_pointer`` - the host pointer to read from.
-  ``size`` - the size (in bytes) to copy from ``host_pointer`` to
   ``buffer``.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``offset`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``offset`` plus ``size`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``host_pointer`` is NULL, ``mux_error_invalid_value``
   **must** be returned.
-  If ``size`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``size`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandWriteBuffer()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` and ``buffer`` passed to
   ``muxCommandWriteBuffer()`` **must** have been created using the same
   ``mux_device_t``.
-  The ``buffer`` passed to ``muxCommandWriteBuffer()`` **must** be bound
   to a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``buffer`` passed to ``muxCommandWriteBuffer()`` **must not** be
   re-bound to a new ``mux_memory_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandWriteBufferRegions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Push a command to a command buffer to read regions from a host pointer to
regions in a ``mux_buffer_t``.

.. code:: c

   mux_result_t muxCommandWriteBufferRegions(
       mux_command_buffer_t command_buffer,
       mux_buffer_t buffer,
       void* host_pointer,
       mux_buffer_region_info_t* regions,
       uint64_t regions_length,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be written to.
-  ``host_pointer`` - the host pointer to read from.
-  ``regions`` - the regions of the ``host_pointer`` to write to regions
   of the ``buffer``.
-  ``regions_length`` - the number of elements in ``regions``.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``host_pointer`` is NULL, ``mux_error_invalid_value``
   **must** be returned.
-  If ``regions`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If any of the ``regions`` contain a ``region`` that has an
   ``x``,\ ``y``, or ``z`` attribute set to zero
   ``mux_error_invalid_value`` **must** be returned. For a 2D
   region ``z`` should be 1, for a 1D region ``y`` and ``z`` should be
   1.
-  If any of the ``src_desc`` or ``dst_desc`` contain an ``x`` or ``y``
   value that is less than the ``x`` or ``y`` value in ``region``,
   ``mux_error_invalid_value`` **must** be returned.
-  If the ``region`` offset by the ``dst_offset`` is out of the bounds
   of the ``buffer``, ``mux_error_invalid_value`` **must** be
   returned.
-  If any of the ``regions`` offset by the ``dst_offset`` overlap, then
   ``mux_error_invalid_value`` **must** be returned.
-  If ``regions_length`` is 0, ``mux_error_invalid_value``
   **must** be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandWriteBufferRegions()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` and ``buffer`` passed to
   ``muxCommandWriteBufferRegions()`` **must** have been created using the
   same ``mux_device_t``.
-  The ``buffer`` passed to ``muxCommandWriteBufferRegions()`` **must** be
   bound to a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``buffer`` passed to ``muxCommandWriteBufferRegions()`` **must not**
   be re-bound to a new ``mux_memory_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandCopyBuffer
~~~~~~~~~~~~~~~~~~~~

Push a command to a command buffer to copy between two
``mux_buffer_t``\ ’s.

.. code:: c

   mux_result_t muxCommandCopyBuffer(
       mux_command_buffer_t command_buffer,
       mux_buffer_t src_buffer,
       uint64_t src_offset,
       mux_buffer_t dst_buffer,
       uint64_t dst_offset,
       uint64_t size,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``src_buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be read from.
-  ``src_offset`` - the offset (in bytes) into ``src_buffer`` to read
   from.
-  ``dst_buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be written to.
-  ``src_offset`` - the offset (in bytes) into ``dst_buffer`` to write
   to.
-  ``size`` - the size (in bytes) to copy from ``src_buffer`` to
   ``dst_buffer``.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``src_offset`` is greater than the size of ``src_buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``src_offset`` plus ``size`` is greater than the size of
   ``src_buffer``, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``dst_offset`` is greater than the size of ``dst_buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``dst_offset`` plus ``size`` is greater than the size of
   ``dst_buffer``, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``size`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``size`` is greater than either the size of ``src_buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``size`` is greater than either the size of ``dst_buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandCopyBuffer()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` and ``src_buffer`` passed to
   ``muxCommandCopyBuffer()`` **must** have been created using the same
   ``mux_device_t``.
-  The ``command_buffer`` and ``dst_buffer`` passed to
   ``muxCommandCopyBuffer()`` **must** have been created using the same
   ``mux_device_t``.
-  The ``src_buffer`` passed to ``muxCommandCopyBuffer()`` **must** be
   bound to a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``src_buffer`` passed to ``muxCommandCopyBuffer()`` **must not** be
   re-bound to a new ``mux_memory_t``.
-  The ``dst_buffer`` passed to ``muxCommandCopyBuffer()`` **must** be
   bound to a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``dst_buffer`` passed to ``muxCommandCopyBuffer()`` **must not** be
   re-bound to a new ``mux_memory_t``.
-  The ``src_buffer`` and ``dst_buffer`` passed to
   ``muxCommandCopyBuffer()`` **must not** refer to overlapping regions
   within the same ``mux_memory_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandCopyBufferRegions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Push a command to a command buffer to copy regions from a
``mux_buffer_t`` to regions in another ``mux_buffer_t``.

.. code:: c

   mux_result_t muxCommandCopyBufferRegions(
       mux_command_buffer_t command_buffer,
       mux_buffer_t src_buffer,
       mux_buffer_t dst_buffer,
       mux_buffer_region_info_t* regions,
       uint64_t regions_length,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``src_buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be read from.
-  ``dst_buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be written to.
-  ``regions`` - the regions of the ``host_pointer`` to write to regions
   of the ``buffer``.
-  ``regions_length`` - the number of elements in ``regions``.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``host_pointer`` is NULL, ``mux_error_invalid_value``
   **must** be returned.
-  If ``regions`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If any of the ``dst_offset``\ ’s in ``regions`` are greater than the
   size of ``buffer``, ``mux_error_invalid_value`` **must** be
   returned.
-  If any of the ``dst_offset``\ ’s plus ``size``\ s in ``regions`` are
   greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If any of the ``size``\ ’s in ``regions`` is 0,
   ``mux_error_invalid_value`` **must** be returned.
-  If any of the ``size``\ ’s is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If any two regions in ``regions`` have a ``dst_offset`` to
   ``dst_offset`` + ``size`` range that overlaps,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``regions_length`` is 0, ``mux_error_invalid_value``
   **must** be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandCopyBufferRegions()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` and ``src_buffer`` passed to
   ``muxCommandCopyBufferRegions()`` **must** have been created using the
   same ``mux_device_t``.
-  The ``command_buffer`` and ``dst_buffer`` passed to
   ``muxCommandCopyBufferRegions()`` **must** have been created using the
   same ``mux_device_t``.
-  The ``src_buffer`` passed to ``muxCommandCopyBufferRegions()`` **must**
   be bound to a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``src_buffer`` passed to ``muxCommandCopyBufferRegions()`` **must
   not** be re-bound to a new ``mux_memory_t``.
-  The ``dst_buffer`` passed to ``muxCommandCopyBufferRegions()`` **must**
   be bound to a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``dst_buffer`` passed to ``muxCommandCopyBufferRegions()`` **must
   not** be re-bound to a new ``mux_memory_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandFillBuffer
~~~~~~~~~~~~~~~~~~~~

``muxCommandFillBuffer()`` pushes a command to a command buffer to fill a
``mux_buffer_t`` from a host pointer.

.. code:: c

   mux_result_t muxCommandFillBuffer(
       mux_command_buffer_t command_buffer,
       mux_buffer_t buffer,
       uint64_t offset,
       uint64_t size,
       const void* pattern_pointer,
       uint64_t pattern_size,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``buffer`` - a buffer previously created by a call to
   ``muxCreateBuffer()`` that will be written to.
-  ``offset`` - the offset (in bytes) into ``buffer`` to write.
-  ``size`` - the size (in bytes) to write into ``buffer``.
-  ``pattern_pointer`` - the pattern to repeatedly write to ``buffer``,
   this data **must** be copied by the device.
-  ``pattern_size`` - the size (in bytes) of ``pattern_pointer``, the
   maximum size is 128 bytes which is the size of the largest supported
   vector type.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``offset`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``offset`` plus ``size`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``size`` is 0, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``size`` is greater than the size of ``buffer``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``pattern_pointer`` is NULL, ``mux_error_invalid_value``
   **must** be returned.
-  If ``pattern_size`` is 0, ``mux_error_invalid_value`` **must**
   be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandFillBuffer()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` and ``buffer`` passed to
   ``muxCommandFillBuffer()`` **must** have been created using the same
   ``mux_device_t``.
-  ``pattern_pointer`` **may** be reused immediately following return
   from the ``muxCommandFillBuffer()`` call.
-  The ``buffer`` passed to ``muxCommandFillBuffer()`` **must** be bound to
   a ``mux_memory_t`` via ``muxBindBufferMemory()``.
-  The ``buffer`` passed to ``muxCommandFillBuffer()`` **must not** be
   re-bound to a new ``mux_memory_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandReadImage
~~~~~~~~~~~~~~~~~~~

Push a read image command to the command buffer.

.. code:: c

   mux_result_t muxCommandReadImage(
       mux_command_buffer_t command_buffer,
       mux_image_t image,
       mux_offset_3d_t offset,
       mux_extent_3d_t extent,
       uint64_t row_size,
       uint64_t slice_size,
       void* pointer,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - The command buffer to push the image read command
   to.
-  ``image`` - The image to read from.
-  ``offset`` - The x, y, z, offset in pixels into the image to read
   from.
-  ``extent`` - The width, height, depth in pixels of the image to read
   from.
-  ``row_size`` - The row size in bytes of the host addressable pointer
   data.
-  ``slice_size`` - The slice size in bytes of the host addressable
   pointer data.
-  ``pointer`` - The host addressable pointer to image data to write
   into.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when;

   -  ``command_buffer`` is ``NULL``.
   -  ``image`` is ``NULL``.
   -  ``offset`` and ``extent`` combined exceed the size of the image.
   -  ``pointer`` is ``NULL``.
   -  ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not
      ``NULL``.
   -  ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is
      ``NULL``.

-  Otherwise ``mux_success`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCommandReadImage()``, for a given ``command_buffer``,
   **shall** be considered thread-safe.
-  ``command_buffer`` and ``image`` **must** have been created using the
   same
-  ``mux_device_t``.
-  ``image`` **must** be bound to a valid ``mux_memory_t`` instance.
-  ``image`` **must not** be rebound to another ``mux_memory_t`` until
   ``command_buffer`` completes execution.
-  ``pointer`` **must** remain valid until ``command_buffer`` completes
   execution.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandWriteImage
~~~~~~~~~~~~~~~~~~~~

Push a write image command to the command buffer.

.. code:: c

   mux_result_t muxCommandWriteImage(
       mux_command_buffer_t command_buffer,
       mux_image_t image,
       mux_offset_3d_t offset,
       mux_extent_3d_t extent,
       uint64_t row_size,
       uint64_t slice_size,
       const void* pointer,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - The command buffer to push the image write command
   to.
-  ``image`` - The image to write to.
-  ``offset`` - The x, y, z, offset in pixel into the image to write to.
-  ``extent`` - The width, height, depth in pixels of the image to write
   to.
-  ``row_size`` - The row size in bytes of the host addressable pointer
   data.
-  ``slice_size`` - The slice size in bytes of the host addressable
   pointer data.
-  ``pointer`` - The host addressable pointer to image data to read
   from.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when;

   -  ``command_buffer`` is ``NULL``.
   -  ``image`` is ``NULL``.
   -  ``offset`` and ``extent`` combined exceed the size of the image.
   -  ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not
      ``NULL``.
   -  ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is
      ``NULL``.

-  Otherwise ``mux_success`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCommandWriteImage()``, for a given ``command_buffer``,
   **shall** be considered thread-safe.
-  ``command_buffer`` and ``image`` **must** have been created using the
   same ``mux_device_t``.
-  ``image`` **must** be bound to a valid ``mux_memory_t`` instance.
-  ``image`` **must not** be rebound to another ``mux_memory_t`` until
   ``command_buffer`` completes execution.
-  ``pointer`` **must** remain valid until ``command_buffer`` completes
   execution.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandFillImage
~~~~~~~~~~~~~~~~~~~

Push a fill image command to the command buffer.

.. code:: c

   mux_result_t muxCommandFillImage(
       mux_command_buffer_t command_buffer,
       mux_image_t image,
       const void* color,
       uint32_t color_size,
       mux_offset_3d_t offset,
       mux_extent_3d_t extent,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - The command buffer to push the image fill command
   to.
-  ``image`` - The image to fill.
-  ``color`` - The color to fill the image with, this data **must** be
   copied by the device.
-  ``color_size`` - The size (in bytes) of color, the maximum size is 16
   bytes which is the size of a 4 element vector of 32-bit elements.
-  ``offset`` - The x, y, z, offset in pixels into the image to fill.
-  ``extent`` - The width, height, depth in pixels of the image to fill.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when;

   -  ``command_buffer`` is ``NULL``.
   -  ``image`` is ``NULL``.
   -  ``color`` is ``NULL``.
   -  ``color_size`` is ``0`` or is greater than ``16``.
   -  ``offset`` combined with ``extent`` is greater than the image
      size.
   -  ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not
      ``NULL``.
   -  ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is
      ``NULL``.

-  Otherwise ``mux_success`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCommandFillImage()``, for a given ``command_buffer``,
   **shall** be considered thread-safe.
-  ``command_buffer`` and ``image`` **must** have been created using the
   same ``mux_device_t``.
-  ``image`` **must** be bound to a valid ``mux_memory_t`` instance.
-  ``image`` **must not** be rebound to another ``mux_memory_t`` until
   ``command_buffer`` completes execution.
-  ``color`` **may** be used immediately after ``muxCommandFillImage()``
   returns.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandCopyImage
~~~~~~~~~~~~~~~~~~~

Push a copy image command to the command buffer.

.. code:: c

   mux_result_t muxCommandCopyImage(
       mux_command_buffer_t command_buffer,
       mux_image_t src_image,
       mux_image_t dst_image,
       mux_offset_3d_t src_offset,
       mux_offset_3d_t dst_offset,
       mux_extent_3d_t extent,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - The command buffer to push the image copy command
   to.
-  ``src_image`` - The source image where data will be copied from.
-  ``dst_image`` - The destination image where data will be copied to.
-  ``src_offset`` - The x, y, z offset in pixels into the source image.
-  ``dst_offset`` - The x, y, z offset in pixels into the destination
   image.
-  ``extent`` - The width, height, depth in pixels range of the images
   to be copied.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when;

   -  ``command_buffer`` is ``NULL``.
   -  ``src_image`` is ``NULL``.
   -  ``dst_image`` is ``NULL``.
   -  ``src_offset`` combined with ``extent`` is greater than
      ``src_image``\ ’s size.
   -  ``dst_offset`` combined with ``extent`` is greater than
      ``dst_image``\ ’s size.
   -  ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not
      ``NULL``.
   -  ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is
      ``NULL``.

-  Otherwise ``mux_success`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCommandCopyImage()``, for a given ``command_buffer``,
   **shall** be considered thread-safe.
-  ``command_buffer``, ``src_image`` and ``dst_image`` **must** have been
   created using the same ``mux_device_t``.
-  ``src_image`` **must** be bound to a valid ``mux_memory_t`` instance.
-  ``src_image`` **must not** be rebound to another ``mux_memory_t``
   until ``command_buffer`` completes execution.
-  ``dst_image`` **must** be bound to a valid ``mux_memory_t`` instance.
-  ``dst_image`` **must not** be rebound to another ``mux_memory_t``
   until ``command_buffer`` completes execution.
-  ``src_image`` and ``dst_image`` **must not** refer to overlapping
   regions within the same ``mux_memory_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandCopyImageToBuffer
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Push a copy image to buffer command to the command buffer.

.. code:: c

   mux_result_t muxCommandCopyImageToBuffer(
       mux_command_buffer_t command_buffer,
       mux_image_t src_image,
       mux_buffer_t dst_buffer,
       mux_offset_3d_t src_offset,
       uint64_t dst_offset,
       mux_extent_3d_t extent,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - The command buffer to push the image to buffer
   copy to.
-  ``src_image`` - The source image to copy data from.
-  ``dst_buffer`` - The destination buffer to copy data to.
-  ``src_offset`` - The x, y, z, offset in pixels into the source image
   to copy.
-  ``dst_offset`` - The offset in bytes into the destination buffer to
   copy.
-  ``extent`` - The width, height, depth in pixels of the image to copy.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when;

   -  ``command_buffer`` is ``NULL``.
   -  ``src_image`` is ``NULL``.
   -  ``dst_image`` is ``NULL``.
   -  ``src_offset`` combined with ``extent`` is greater than
      ``src_image`` size.
   -  ``dst_offset`` combined with ``extent`` is greater than
      ``dst_buffer`` size.
   -  ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not
      ``NULL``.
   -  ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is
      ``NULL``.

-  Otherwise ``mux_success`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCommandCopyImageToBuffer()``, for a given
   ``command_buffer``, **shall** be considered thread-safe.
-  ``command_buffer`` and ``image`` **must** have been created using the
   same ``mux_device_t``.
-  ``src_image`` **must** be bound to a valid ``mux_memory_t`` instance.
-  ``src_image`` **must not** be rebound to another ``mux_memory_t``
   until ``command_buffer`` completes execution.
-  ``dst_buffer`` **must** be bound to a valid ``mux_memory_t``
   instance.
-  ``dst_buffer`` **must not** be rebound to another ``mux_memory_t``
   until ``command_buffer`` completes execution.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandCopyBufferToImage
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Push a copy buffer to image command to the command buffer.

.. code:: c

   mux_result_t muxCommandCopyBufferToImage(
       mux_command_buffer_t command_buffer,
       mux_buffer_t src_buffer,
       mux_image_t dst_image,
       uint32_t src_offset,
       mux_offset_3d_t dst_offset,
       mux_extent_3d_t extent,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - The command buffer to push the buffer to image
   copy command to.
-  ``src_buffer`` - The source buffer to copy data from.
-  ``dst_image`` - The destination image to copy data to.
-  ``src_offset`` - The offset in bytes into the source buffer to copy.
-  ``dst_offset`` - The x, y, z, offset in pixels into the destination
   image to copy.
-  ``extent`` - The width, height, depth in pixels range of the data to
   copy.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  ``mux_error_invalid_value`` **must** be returned when;

   -  ``command_buffer`` is ``NULL``.
   -  ``src_buffer`` is ``NULL``.
   -  ``dst_image`` is ``NULL``.
   -  ``src_offset`` combined with ``extent`` is greater than
      ``src_buffer`` size.
   -  ``dst_offset`` combined with ``extent`` is greater than
      ``dst_image`` size.
   -  ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not
      ``NULL``.
   -  ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is
      ``NULL``.

-  Otherwise ``mux_success`` **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCommandCopyBufferToImage()``, for a given
   ``command_buffer``, **shall** be considered thread-safe.
-  ``command_buffer`` and ``image`` **must** have been created using the
   same ``mux_device_t``.
-  ``src_buffer`` **must** be bound to a valid ``mux_memory_t``
   instance.
-  ``src_buffer`` **must not** be rebound to another ``mux_memory_t``
   until ``command_buffer`` completes execution.
-  ``dst_image`` **must** be bound to a valid ``mux_memory_t`` instance.
-  ``dst_image`` **must not** be rebound to another ``mux_memory_t``
   until ``command_buffer`` completes execution.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandNDRange
~~~~~~~~~~~~~~~~~

``muxCommandNDRange()`` pushes a command to a command buffer to execute a
kernel.

.. code:: c

   mux_result_t muxCommandNDRange(
       mux_command_buffer_t command_buffer,
       mux_kernel_t kernel,
       mux_ndrange_options_t options,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``kernel`` - a kernel previously created by a call to ``muxCreateKernel()``.
-  ``options`` - a ``mux_ndrange_options_t`` with user provided
   kernel execution options.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``options.descriptors`` is not NULL and ``descriptors_length`` is 0,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``options.descriptors`` is NULL and ``descriptors_length`` is not 0,
   ``mux_error_invalid_value`` **must** be returned.
-  If any element in ``options.local_size`` is 0, ``mux_error_invalid_value``
   **must** be returned.
-  If ``options.global_offset`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``options.global_size`` is NULL, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``options.length`` is 0 or greater than 3, ``mux_error_invalid_value``
   **must** be returned.
-  If ``options.descriptors`` contains an element where the ``type`` data member
   is ``mux_descriptor_info_type_custom_buffer`` and
   ``device->info->custom_buffer_capabilities`` is ``0``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandNDRange()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` and ``kernel`` passed to ``muxCommandNDRange()``
   **must** have been created using the same ``mux_device_t``.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxUpdateDescriptors
~~~~~~~~~~~~~~~~~~~~

``muxUpdateDescriptors`` updates the ``mux_descriptor_info_t`` descriptors
associated with an ND range command enqueued via ``muxCommandNDRange()``. This
entry point updates arguments to an ND range in a command buffer.

Entry point is optional as it requires device support for updating descriptors and
**must** return ``mux_error_feature_unsupported`` if this is not the case.

.. code:: c

   mux_result_t muxUpdateDescriptors(mux_command_buffer_t command_buffer,
                                    mux_command_id_t command_id,
                                    uint64_t num_args, uint64_t *arg_indices,
                                    mux_descriptor_info_t *descriptors);

- ``command_buffer`` - the command buffer containing the NDRange command.
- ``command_id`` - the ID that uniquely identifies the NDRange command within
                 ``command_buffer`` by the index of command within the command
                 buffer.
- ``num_args`` - the number of arguments to be updated.
- ``arg_indices`` - the indices of the arguments to be updated.
- ``descriptors`` - the descriptors for the new arguments.

.. rubric:: Return Codes

- If the device associated with ``command_buffer`` does not support updating
  descriptors i.e. ``mux_device_info_s::descriptors_updatable == false``
  ``mux_error_feature_unsupported`` must be returned.
- Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned, ``command_buffer``
**should** be considered unchanged and the arguments to the ND Range command
identified by ``command_id`` **should** be considered unchanged.

.. rubric:: Valid Usage

- Calls to ``muxUpdateDescriptors()`` operating on distinct ``command_buffer``'s
  **shall** be considered thread-safe.
- The ``command_buffer`` **shall** be in the *finalized* state when it
  passed to ``muxUpdateDescriptors()``.
- The type of a descriptor **should not** be changed unless it is between:
  ``mux_descriptor_info_type_buffer`` and
  ``mux_descriptor_info_type_null_buffer``.
- If any descriptors of type ``mux_descriptor_info_type_plain_old_data``
  change their size, the behaviour is undefined.

muxCommandUserCallback
~~~~~~~~~~~~~~~~~~~~~~

``muxCommandUserCallback()`` pushes a command to a command buffer that
executes a user callback.

.. code:: c

   mux_result_t muxCommandUserCallback(
       mux_command_buffer_t command_buffer,
       mux_command_user_callback_t user_function,
       void* user_data,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``user_function`` - a user callback function.
-  ``user_data`` - some user data that will be passed to the
   ``user_function`` when it is executed.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``user_function`` is NULL, ``mux_error_invalid_value``
   **must** be returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``command_buffer`` **should** be considered unchanged.

.. rubric:: Valid Usage

-  Calls to ``muxCommandUserCallback()`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandBeginQuery
~~~~~~~~~~~~~~~~~~~~

Push a begin query command to the command buffer, enable a query pool for
use storing query results.

.. code:: c

   mux_result_t muxCommandBeginQuery(
       mux_command_buffer_t command_buffer,
       mux_query_pool_t query_pool,
       uint32_t query_index,
       uint32_t query_count,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - the command buffer to push the begin query command
   to.
-  ``query_pool`` - the query pool to store the query result in.
-  ``query_index`` - the first query slot index that will contain
   results.
-  ``query_count`` - the number of query slots that will contain
   results.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``command_buffer`` is not a valid ``mux_command_buffer_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_pool`` is not a valid ``mux_query_pool_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_index`` is greater than or equal to ``query_pool->count``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_index + query_count`` is greater than
   ``query_pool->count``, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCommandBeginQuery`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandEndQuery
~~~~~~~~~~~~~~~~~~

Push an end query command to the command buffer. End query commands disable a
query pool from use for storing query results.

.. code:: c

   mux_result_t muxCommandEndQuery(
       mux_command_buffer_t command_buffer,
       mux_query_pool_t query_pool,
       uint32_t query_index,
       uint32_t query_count,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - the command buffer to push the begin query command
   to.
-  ``query_pool`` - the query pool the result is stored in.
-  ``query_index`` - the first query slot index that contains results.
-  ``query_count`` - the number of query slots that contains results.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``command_buffer`` is not a valid ``mux_command_buffer_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_pool`` is not a valid ``mux_query_pool_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_index`` is greater than or equal to ``query_pool->count``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_index + query_count`` is greater than
   ``query_pool->count``, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``query_pool`` is not currently enabled with a previous call to
   ``muxCommandBeginQuery()`` with the same ``command_buffer``,
   ``mux_error_invalid_value`` **should** be returned.
-  If ``query_index`` is not currently enabled with a previous call to
   ``muxCommandBeginQuery()`` with the same ``command_buffer`` and
   ``query_pool``, ``mux_error_invalid_value`` **should** be
   returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

..

   Note: ``query_pool`` and ``query_index`` validation relating to
   previous calls to ``muxCommandBeginQuery()`` **should** be performed by
   the target.

.. rubric:: Valid Usage

-  Calls to ``muxCommandEndQuery`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

muxCommandResetQueryPool
~~~~~~~~~~~~~~~~~~~~~~~~

Push a reset query pool command to the command buffer. Reset query pool
commands enable reuse of the query pool as if it was newly created.

.. code:: c

   mux_result_t muxCommandResetQueryPool(
       mux_command_buffer_t command_buffer,
       mux_query_pool_t query_pool,
       uint32_t query_index,
       uint32_t query_count,
       uint32_t num_sync_points_in_wait_list,
       const mux_sync_point_t* sync_point_wait_list,
       mux_sync_point_t* sync_point);

-  ``command_buffer`` - the command buffer to push the query pool reset
   command to.
-  ``query_pool`` - the query pool to reset.
-  ``query_index`` - the first query index to reset.
-  ``query_count`` - the number of query slots to reset.
-  ``num_sync_points_in_wait_list`` - Number of items in
   ``sync_point_wait_list``.
-  ``sync_point_wait_list`` - List of sync-points that need to complete before
   this command can be executed.
-  ``sync_point`` - Returns a sync-point identifying this command, which **may**
   be passed as NULL, that other commands in the command-buffer can wait on.

.. rubric:: Return Codes

-  If ``command_buffer`` is not a valid ``mux_command_buffer_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_pool`` is not a valid ``mux_query_pool_t``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_index`` is greater than or equal to ``query_pool->count``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``query_index + query_count`` is greater than
   ``query_pool->count``, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``num_sync_points_in_wait_list`` is 0 and ``sync_point_wait_list`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``num_sync_points_in_wait_list`` is not 0 and ``sync_point_wait_list`` is NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxCommandResetQueryPool`` operating on distinct
   ``command_buffer``\ ’s **shall** be considered thread-safe.
-  The ``command_buffer`` argument **must** not be in the *finalized* state.
-  The elements of ``sync_point_wait_list`` **must** have been created from
   commands recorded to ``command_buffer``.

Fences
------

Fences in Mux allow one way synchronization from device to host.

.. todo::

   Expand on this once APIs for waiting on fences have been added (see
   CA-4270).


muxCreateFence
~~~~~~~~~~~~~~

``muxCreateFence()`` creates a fence from a given Mux device.

.. code:: c

   mux_result_t muxCreateFence(
       mux_device_t device,
       mux_allocator_info_t allocator_info,
       mux_fence_t* out_fence);

-  ``device`` - the ``mux_device_t`` to create this fence with.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_fence`` - the newly created fence.

.. rubric:: Return Codes

-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_fence`` is NULL, ``mux_error_null_out_parameter``
   **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_fence`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxCreateFence()`` **shall** be considered
   thread-safe.

muxDestroyFence
~~~~~~~~~~~~~~~

``muxDestroyFence()`` destroys a previously created
``mux_fence_t``.

.. code:: c

   void muxDestroyFence(
       mux_device_t device,
       mux_fence_t fence,
       mux_allocator_info_t allocator_info);

-  ``device`` - the device the fence was created with.
-  ``fence`` - a fence previously created by a call to
   ``muxCreateFence()``.
-  ``allocator_info`` - the user provided allocator ``fence`` was
   created with.

.. rubric:: Valid Usage

-  Calls to ``muxDestroyFence()`` with a given ``fence``
   **shall** be considered thread-safe.
-  The ``allocator_info`` passed to ``muxDestroyFence()`` **must**
   be the same ``mux_allocator_info_t`` as was used in the call to
   ``muxCreateFence()`` that created ``fence``.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``fence`` **must** be a valid ``mux_fence_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

muxResetFence
~~~~~~~~~~~~~

``muxResetFence()`` resets a previously created ``mux_fence_t``
such that it is in the same non-signalled state as when it was
originally created.

.. code:: c

   mux_result_t muxResetFence(
       mux_fence_t fence);

-  ``fence`` - a fence previously created by a call to
   ``muxCreateFence()``.

.. rubric:: Return Codes

-  ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxResetFence()`` with a given ``fence`` **shall**
   be considered thread-safe.

.. rubric:: Validation Rules

-  The ``mux_device_t`` that was used in the creation of the
   ``mux_fence_t`` **must not** be destroyed before the fence
   has been destroyed.

Semaphores
----------

Semaphores in Mux allow synchronization between
``mux_command_buffer_t``\ ’s executing on a ``mux_queue_t``. When a
``mux_command_buffer_t`` is dispatched via ``muxDispatch()`` a user can
request that ``mux_command_buffer_t`` only begin execution once a number
of semaphores are signalled, and can request that when a
``mux_command_buffer_t`` has completed that it signal a number of
semaphores too.

muxCreateSemaphore
~~~~~~~~~~~~~~~~~~

``muxCreateSemaphore()`` creates a semaphore from a given Mux device.

.. code:: c

   mux_result_t muxCreateSemaphore(
       mux_device_t device,
       mux_allocator_info_t allocator_info,
       mux_semaphore_t* out_semaphore);

-  ``device`` - the ``mux_device_t`` to create this semaphore with.
-  ``allocator_info`` - the user provided allocator **should** be used
   for host memory allocations as described in the
   `Allocators <#allocators>`__ section.
-  ``out_semaphore`` - the newly created semaphore.

.. rubric:: Return Codes

-  If ``allocator_info`` function pointer fields are ``NULL``,
   ``mux_error_null_allocator_callback`` **must** be returned.
-  If ``out_semaphore`` is NULL, ``mux_error_null_out_parameter``
   **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_semaphore`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxCreateSemaphore()`` **shall** be considered
   thread-safe.

muxDestroySemaphore
~~~~~~~~~~~~~~~~~~~

``muxDestroySemaphore()`` destroys a previously created
``mux_semaphore_t``.

.. code:: c

   void muxDestroySemaphore(
       mux_device_t device,
       mux_semaphore_t semaphore,
       mux_allocator_info_t allocator_info);

-  ``device`` - the device the semaphore was created with.
-  ``semaphore`` - a semaphore previously created by a call to
   ``muxCreateSemaphore()``.
-  ``allocator_info`` - the user provided allocator ``semaphore`` was
   created with.

.. rubric:: Valid Usage

-  Calls to ``muxDestroySemaphore()`` with a given ``semaphore``
   **shall** be considered thread-safe.
-  The ``allocator_info`` passed to ``muxDestroySemaphore()`` **must**
   be the same ``mux_allocator_info_t`` as was used in the call to
   ``muxCreateSemaphore()`` that created ``semaphore``.
-  ``muxDestroySemaphore()`` **must not** be called in a
   ``muxDispatch()``\ ’s ``user_function`` callback.
-  ``muxDestroySemaphore()`` **may** be called as soon as a
   ``muxDispatch()`` has signalled the semaphore.
-  ``device`` **must** be a valid ``mux_device_t``.
-  ``semaphore`` **must** be a valid ``mux_semaphore_t``.
-  ``allocator_info`` **must** be a valid ``mux_allocator_info_t``.

muxResetSemaphore
~~~~~~~~~~~~~~~~~

``muxResetSemaphore()`` resets a previously created ``mux_semaphore_t``
such that it is in the same non-signalled state as when it was
originally created.

.. code:: c

   mux_result_t muxResetSemaphore(
       mux_semaphore_t semaphore);

-  ``semaphore`` - a semaphore previously created by a call to
   ``muxCreateSemaphore()``.

.. rubric:: Return Codes

-  ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxResetSemaphore()`` with a given ``semaphore`` **shall**
   be considered thread-safe.
-  ``muxResetSemaphore()`` **must not** be called in a
   ``muxDispatch()``\ ’s ``user_function`` callback.
-  ``muxResetSemaphore()`` **should not** be called with a given
   ``semaphore`` that is being waited on by a call ``muxDispatch()``,
   but has not been signalled yet.
-  ``muxResetSemaphore()`` **may** be called as soon as a
   ``muxDispatch()`` has signalled the semaphore.

.. rubric:: Validation Rules

-  The ``mux_device_t`` that was used in the creation of the
   ``mux_semaphore_t`` **must not** be destroyed before the semaphore
   has been destroyed.
-  The ``mux_semaphore_t`` **must not** be reset while a
   ``mux_command_buffer_t`` that it is waiting on is currently executing.
-  The ``mux_semaphore_t`` **must not** be reset while a
   ``mux_command_buffer_t`` that is signalling it currently executing.

Queues
------

Queues in Mux are where ``mux_command_buffer_t`` are dispatched to the
device’s hardware for execution.

muxGetQueue
~~~~~~~~~~~

``muxGetQueue()`` retrieves a queue from a given Mux device.

.. code:: c

   mux_result_t muxGetQueue(
       mux_device_t device,
       mux_queue_type_e queue_type,
       uint32_t queue_index,
       mux_queue_t* out_queue);

-  ``device`` - the ``mux_device_t`` to get this queue from.
-  ``queue_type`` - the type of queue to get.
-  ``queue_index`` - the index of the queue type to get.
-  ``out_queue`` - the retrieved queue.

.. rubric:: Return Codes

-  If ``queue_type`` is not a valid ``mux_queue_type_e``,
   ``mux_error_invalid_value`` **must** be returned.
-  If ``queue_index`` is not a valid index for ``device``\ ’s
   ``mux_queue_t``\ ’s, ``mux_error_invalid_value`` **must** be
   returned.
-  If ``out_queue`` is NULL, ``mux_error_null_out_parameter`` **must**
   be returned.
-  Otherwise ``mux_success`` **should** be returned.

If an error code other than ``mux_success`` is returned,
``out_queue`` **must not** be used.

.. rubric:: Valid Usage

-  Calls to ``muxGetQueue()`` **shall** be considered thread-safe.

muxDispatch
~~~~~~~~~~~

``muxDispatch()`` enqueues a ``mux_command_buffer_t`` for executing on a
given queue. Commands within a command buffer **must** be executed *as
if* in the order they are pushed onto the command buffer.

.. code:: c

   mux_result_t muxDispatch(
       mux_queue_t queue,
       mux_command_buffer_t command_buffer,
       mux_fence_t fence,
       mux_semaphore_t* wait_semaphores,
       uint32_t wait_semaphores_length,
       mux_semaphore_t* signal_semaphores,
       uint32_t signal_semaphores_length
       void (*user_function)(
           mux_command_buffer_t command_buffer,
           mux_result_t error,
           void* const user_data),
       void* user_data);

-  ``queue`` - a queue previously created by a call to
   ``muxGetQueue()``.
-  ``command_buffer`` - a command buffer previously created by a call to
   ``muxCreateCommandBuffer()``.
-  ``fence`` - an optional fence created by a call to ``muxCreateFence()`` to
   be signaled when ``command_buffer`` completes.
-  ``wait_semaphores`` - an array of semaphores that **must** be
   complete before ``command_buffer`` begins execution.
-  ``wait_semaphores_length`` - the length of ``wait_semaphores``.
-  ``signal_semaphores`` - an array of semaphores that **must** be
   signaled once both ``command_buffer`` and ``user_function`` complete
   execution.
-  ``signal_semaphores_length`` - the length of ``signal_semaphores``.
-  ``user_function`` - a callback function to call when the command
   group has completed, this **must** occur before ``signal_semaphores``
   are signaled.
-  ``user_data`` - the user data to pass to ``user_function``.

.. rubric:: Return Codes

-  If ``wait_semaphores`` is NULL and ``wait_semaphores_length`` is not
   0, ``mux_error_invalid_value`` **must** be returned.
-  If ``wait_semaphores`` is not NULL and ``wait_semaphores_length`` is
   0, ``mux_error_invalid_value`` **must** be returned.
-  If ``signal_semaphores`` is NULL and ``signal_semaphores_length`` is
   not 0, ``mux_error_invalid_value`` **must** be returned.
-  If ``signal_semaphores`` is not NULL and ``signal_semaphores_length``
   is 0, ``mux_error_invalid_value`` **must** be returned.
-  If ``user_function`` is NULL and ``user_data`` is not NULL,
   ``mux_error_invalid_value`` **must** be returned.
-  Otherwise ``mux_success`` **should** be returned.
-  If ``command_buffer`` is not finalized, ``mux_error_invalid_value``
   **must** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxDispatch()`` **shall** be considered thread-safe.
-  The ``queue`` and ``command_buffer`` passed to ``muxDispatch()``
   **must** have been created using the same ``mux_device_t``.
-  The ``queue`` and ``fence`` passed to ``muxDispatch()``
   **must** have been created using the same ``mux_device_t``.
-  The ``queue`` and all ``mux_semaphore_t``\ ’s in ``wait_semaphores``
   **must** have been created using the same ``mux_device_t``.
-  The ``queue`` and all ``mux_semaphore_t``\ ’s in
   ``signal_semaphores`` **must** have been created using the same
   ``mux_device_t``.
-  All ``mux_semaphore_t``\ ’s in ``signal_semaphores`` **must not** be
   signalled by another call to ``muxDispatch()`` while a given
   ``command_buffer`` is currently executing.
-  All ``mux_semaphore_t``\ ’s in ``signal_semaphores`` **may** be reset
   with ``muxResetSemaphore()`` or destroyed with
   ``muxDestroySemaphore()`` as soon as the command buffer invoked by
   ``muxDispatch()`` has completed.
-  ``fence`` **may** be reset with ``muxResetFence()`` or destroyed with
   ``muxDestroyFence()`` as soon as the command buffer invoked by
   ``muxDispatch()`` has completed.
-  The ``command_buffer`` passed to ``muxDispatch()`` **may** be empty.
-  The ``fence`` passed to ``muxDispatch()`` **may** be null indicating there
   is no fence to be signaled on dispatch completion.
-  The ``user_function`` callback **must not** destroy or reset the
   dispatched ``command_buffer`` or the semaphores in the
   ``wait_semaphores`` and ``signal_semaphores`` lists.
-  The ``command_buffer`` **shall** be considered complete after the
   ``user_function`` has been called, semaphores in the ``signal_semaphores``
   list have been signaled and the optional fence argument has been signaled if
   it exists.

.. rubric:: Validation Rules

-  A ``mux_command_buffer_t`` that has been dispatched to a queue via
   ``muxDispatch()`` **must not** be destroyed while it is still
   executing.

muxTryWait
~~~~~~~~~~

``muxTryWait()`` attempts to wait on a ``mux_fence_t`` to be signaled on
completion of a ``mux_command_buffer_t`` that is executing on a given queue
dispatched via ``muxDispatch()``.

.. code:: c

   mux_result_t muxTryWait(
       mux_queue_t queue,
       uint64_t timeout,
       mux_fence_t fence);

-  ``queue`` - a queue previously created by a call to
   ``muxGetQueue()``.
-  ``timeout`` - a timeout period in units of nanoseconds. The value of
   ``timeout`` is adjusted to the closest value supported by the target's
   timeout accuracy, this **may** be longer than one nanosecond and hence
   ``muxTryWait`` may wait longer than the period defined by ``timeout``.
-  ``fence`` - a fence previously used in a call to
   ``muxDispatch()`` on the given ``queue``.

.. rubric:: Return Codes

-  If the ``fence`` has not yet been signaled,
   ``mux_fence_not_ready`` **must** be returned.
-  If the ``fence`` has been signaled,
   ``mux_error_fence_failure`` **may** be returned.
-  Otherwise ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxTryWait()`` **shall** be considered thread-safe.
-  The ``queue`` and ``fence`` passed to ``muxTryWait()``
   **must** have been created using the same ``mux_device_t``.
-  The ``fence`` passed to ``muxTryWait()`` **must** have previously been
   dispatched with a command buffer on the given ``queue`` via a call to
   ``muxDispatch()``.
-  ``muxTryWaitTest()`` **may** wait for the suggested ``timeout`` period.
   Passing ``0`` for the ``timeout`` parameter indicates there is no timeout
   period.

muxWait
~~~~~~~

``muxWait()`` waits on a ``mux_fence_t`` to be signaled on completion of a
``mux_command_buffer_t`` that is executing on a given queue dispatched via
``muxDispatch()``.

.. code:: c

   mux_result_t muxWait(mux_queue_t queue, mux_fence_t fence);

-  ``queue`` - a queue previously created by a call to
   ``muxGetQueue()``.
-  ``fence`` - a fence previously used in a call to
   ``muxDispatch()`` on the given ``queue``.

.. rubric:: Return Codes

-  If the ``fence`` has been signaled,
   ``mux_error_fence_failure`` **may** be returned.
-  Otherwise ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxWait()`` **shall** be considered thread-safe.
-  The ``queue`` and ``fence`` passed to ``muxWait()`` **must**
   have been created using the same ``mux_device_t``.
-  The ``fence`` passed to ``muxTryWait()`` **must** have previously been
   dispatched with a command buffer on the given ``queue`` via a call to
   ``muxDispatch()``.

muxWaitAll
~~~~~~~~~~

``muxWaitAll()`` waits on all previously dispatched
``mux_command_buffer_t``\ ’s that are executing on a given queue to
complete.

.. code:: c

   mux_result_t muxWaitAll(
       mux_queue_t queue);

-  ``queue`` - a queue previously created by a call to
   ``muxGetQueue()``.

.. rubric:: Return Codes

-  ``mux_success`` **should** be returned.

.. rubric:: Valid Usage

-  Calls to ``muxWaitAll()`` **shall** be considered thread-safe.

.. rubric:: Validation Rules

-  The ``mux_device_t`` that was used in the creation of the
   ``mux_queue_t`` **must not** be destroyed before the queue has been
   destroyed.
-  The ``mux_queue_t`` **must not** be destroyed while any
   ``mux_command_buffer_t``\ ’s that were previously enqueued for
   execution via ``muxDispatch()`` are currently executing.

Execution Model
---------------

This section briefly details Mux's execution model: how work is defined for a
Mux device, and how the execution of work on that device can be controlled with
the API.

The smallest unit of work that can be created for a device at the Mux API level
is the command. These are the operations defined with the ``muxCommand`` entry
points, they include commands such as :ref:`Buffer
<specifications/mux-runtime-spec:buffers>` operations
(reading/writing/copying), and :ref:`Kernel
<specifications/mux-runtime-spec:kernels>` executions.

Commands are not directly executed, instead they are first added to
:ref:`Command Buffers <specifications/mux-runtime-spec:Command Buffers>`. A
Command Buffer can be thought of as a list of N commands that must be executed
*as if* in the order they were added to the Command Buffer. The *as if* wording
is important: by defining this in-order execution requirement in terms of side
effects rather than absolute execution order, we allow Mux implementations to
execute commands within Command Buffers concurrently as long as there are no
dependencies between them.  Command Buffers are the *executable* unit of work
in the Mux API. Once finalized with a call to ``muxFinalizeCommandBuffer`` a
Command Buffer can be submitted to a :ref:`Queue
<specifications/mux-runtime-spec:queues>` for immediate execution on the
associated Mux device with a ``muxDispatch`` call.

.. .. todo:: Mux Command Buffer lifecycle diagram illustrating the possible
   command buffer states and the ways to transition between them. See CA-3697

Command Buffers executing in a Queue do not have any implicit ordering
guarantees relative to any other executing Command Buffers. Any execution
dependencies between Command Buffers must be manually enforced with the
:ref:`Semaphore <specifications/mux-runtime-spec:semaphores>` primitive. When a
Mux Command Buffer is dispatched, a list of wait Semaphores and a list of
signal Semaphores can optionally also be provided. A Command Buffer will wait
until all of its wait Semaphores are in the signalled state before commencing
execution on the device, and when finished it will put each of its signal
Semaphores into the signalled state. A common use of this mechanism is to
dispatch every Command Buffer with one signal Semaphore, and have it wait on
the signal Semaphores of all currently executing Command Buffers, thus creating
a queue that is guaranteed to execute each of its Command Buffers in the order
they were dispatched.
