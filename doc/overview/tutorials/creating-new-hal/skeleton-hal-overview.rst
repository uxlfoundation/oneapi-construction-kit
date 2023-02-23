Overview of the Skeleton HAL Code
---------------------------------

Now that all of the components used in the tutorial have successfully been built
and running the clik test suite has shown that all tests are failing, the next
step is to begin controlling the RefSi device through the ``refsidrv`` driver in
order to implement all of the required operations defined by the HAL interface.

Before delving into the implementation of these HAL operations, we will briefly
look at the structure of the skeleton HAL source code in order to understand how
the different parts fit together and what needs to be filled in. The skeleton
HAL contains two source files, ``refsi_hal.cpp`` and ``hal_main.cpp``. These
files, along with their headers, are the only files that need to be modified in
order to 'fill in the gaps' needed for a working device HAL.

HAL Basics
^^^^^^^^^^

From an implementer's point of view, the HAL interface is mainly made up of two
C++ classes: ``hal_t`` and ``hal_device_t``. These are the classes the HAL user
interfaces the device with. Since they are abstract classes, they need to be
derived by the implementer and a number of ``virtual`` functions overriden with
code specific to the device the HAL is targeting. In this tutorial we will focus
on these two classes instead of the ancillary HAL structs and types.

Once these classes have been implemented, the device HAL can be built as a
stand-alone shared library that is ready to be consumed by a compute library
such as clik or a ComputeAorta (through ComputeMux). The consumer library will
typically have either a compile-time or run-time variable to select the name of
the device HAL library to load. For clik, this is done through a CMake variable
called ``CLIK_HAL_NAME`` and for the ComputeMux target described in the
following tutorial, ``CA_HAL_NAME``.

.. note::

    The HAL API is designed as a way to quickly write code to interface with a
    device before developing a complete ComputeMux target, not as a programming
    interface. There is nothing preventing an application from using a device
    HAL directly, but that is not the intended use case.

``hal_t``
^^^^^^^^^

``hal_t`` is the first class of the HAL API both implementers and users need to
know about. When a device HAL is built as a shared library, it exposes a single
function: ``get_hal``. This function is responsible for returning an instance of
a ``hal_t``-derived class as well as the HAL API version it implements:

.. code:: c++

    static refsi_tutorial_hal hal_object;

    hal::hal_t *get_hal(uint32_t &api_version) {
      api_version = hal_object.get_info().api_version;
      return &hal_object;
    }

This class reports information related to the HAL 'platform' and to the devices
that the HAL targets, e.g. its name, word size, memory capacity and other
capabilities. It also allows device objects to be created and destroyed. A
platform can be made up of several devices, which could in turn have different
architectures or instead be different units of the same hardware component,
although many HALs will only feature a single device. Several virtual functions
need to be overriden and implemented in order to perform the responsibilities
described above:

.. code:: c++

    /// @brief hal_t provides access to a hardware abstraction layer allowing the
    /// caller to query hal and device information as well as instantiate devices.
    struct hal_t {
      /// @brief Return generic platform information.
      ///
      /// @return Returns a structure with information about the hal.
      virtual const hal_info_t &get_info() = 0;

      /// @brief Return generic target information.
      ///
      /// @param device_index ranges from 0 to hal_manager_info_t::num_devices.
      ///
      /// @return Returns `nullptr` if the operation fails or a pointer to the
      /// device information. The hal retains ownership of the returned pointer and
      /// it does not need to be released. The returned pointer can be upcast
      /// depending on the type information member.
      virtual const hal_device_info_t *device_get_info(uint32_t device_index) = 0;

      /// @brief Request the creation of a new hal device.
      ///
      /// @param device_index ranges from 0 to hal_manager_info_t::num_devices.
      ///
      /// @return Returns `nullptr` if the operation fails.
      virtual hal_device_t *device_create(uint32_t device_index) = 0;

      /// @brief Destroy a device instance.
      ///
      /// @param Device is a currently valid hal_device_t object.
      ///
      /// @return Returns `false` if the operation fails otherwise `true`.
      virtual bool device_delete(hal_device_t *device) = 0;
    };

``hal_main.cpp`` defines the ``refsi_tutorial_hal`` class, which derives from
``hal_t`` described above. This can be seen in the ``refsi_tutorial_hal``
constructor where the ``hal_info`` and ``hal_device_info`` fields are populated.
The ``get_info`` and ``get_device_info`` functions can be used to retrieve these
structures.

``refsi_hal_device`` also implements the ``device_create`` and ``device_delete``
functions. While these functions take an ``index`` parameter in order to support
multiple devices, the RefSi HAL only exposes a single device.

``hal_device_t``
^^^^^^^^^^^^^^^^

``hal_device_t`` is the second HAL class implementers and users need to know
about. It defines a set of abstract functions that perform operations on the
device. These operations are divided into roughly three groups:

* Operations on device memory: allocation, free, copies between host and device.
  Addresses declared with the ``hal_addr_t`` are in the device's address space.
  This means that they cannot be dereferenced directly on the host CPU to access
  the device's memory. Instead, the ``mem_read`` and ``mem_write`` operations
  must be used to transfer data between the device and the host.
* Loading and unloading programs, which contain kernels.
* Kernel execution.

The prototypes for the functions that need to be implemented as a minimum in
order to have a functioning device HAL are shown below:

.. code:: c++

    /// @brief hal_device_t provides direct access to a device exposed by
    /// a hal. it provides access to device memory, program loading, execution
    /// and information queries.
    struct hal_device_t {

      /* Program loading and unloading */

      virtual hal_program_t program_load(const void *data, hal_size_t size) = 0;

      virtual hal_kernel_t program_find_kernel(hal_program_t program,
                                               const char *name) = 0;

      virtual bool program_free(hal_program_t program) = 0;

      /* Operations on device memory */

      virtual hal_addr_t mem_alloc(hal_size_t size, hal_size_t alignment) = 0;

      virtual bool mem_free(hal_addr_t addr) = 0;

      virtual bool mem_read(void *dst, hal_addr_t src, hal_size_t size) = 0;

      virtual bool mem_write(hal_addr_t dst, const void *src, hal_size_t size) = 0;

      /* Kernel execution */

      virtual bool kernel_exec(hal_program_t program, hal_kernel_t kernel,
                               const hal_ndrange_t *nd_range, const hal_arg_t *args,
                               uint32_t num_args, uint32_t work_dim) = 0;

      ...
    }


``refsi_hal.cpp`` defines the ``refsi_hal_device`` class, which derives from
``hal_device_t`` and implements the various device HAL operations we
have mentioned previously. It is also the file which is changed most during the
development of the HAL as will be seen in the following sections of this
tutorial. The device HAL operation functions are initially 'stubs', performing
no operation and returning an error value such as ``false``,
``hal::hal_nullptr`` or ``hal::hal_invalid_program``.

.. note::
    All HAL operation functions need to be safe when called from multiple
    threads. To achieve this, the RefSi HAL includes a global mutex in the
    ``refsi_tutorial_hal`` class. This lock needs to be held when executing any
    HAL operation, which can done by declaring a lock-guard variable with the
    `refsi_locker` type. Examples of this RAII pattern can be found in the
    skeleton HAL, where stub functions in the `refsi_hal_device` use it to lock
    the global mutex.

