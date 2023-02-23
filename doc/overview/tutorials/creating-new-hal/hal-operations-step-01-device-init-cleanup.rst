Device Initialization and Cleanup
---------------------------------

The first HAL operations to implement are device initialization
(``device_create``) and cleanup (``device_free``), which are performed in
``hal_main.cpp``. These operations create and destroy ``refsi_hal_device``
instances, respectively. Another task is opening the RefSi device through the
``refsidrv`` driver in order to retrieve a ``refsi_device_t`` handle, which can
be used to control the device.

First, we will add a field to ``refsi_hal_device`` to store this handle and add
a corresponding parameter to the constructor. We will also add an ``initialize``
function that can be used to perform device initialization tasks that can fail:

.. code:: c++

    // refsi_hal.h
    #include "hal.h"
    #include "hal_riscv.h"
    #include "refsidrv/refsidrv.h"   // Added

    class refsi_hal_device : public hal::hal_device_t {
     public:
      refsi_hal_device(riscv::hal_device_info_riscv_t *info, refsi_device_t device,
                       std::recursive_mutex &hal_lock);
      bool initialize();             // Added
      ...
     private:
      std::recursive_mutex &hal_lock;
      hal::hal_device_info_t *info;
      refsi_device_t device;         // Added
    };

The definition of the constructor needs to be updated as well. In particular,
the ``device`` field needs to be initialized using the new parameter:

.. code:: c++

    // refsi_hal.cpp
    refsi_hal_device::refsi_hal_device(riscv::hal_device_info_riscv_t *info,
                                       refsi_device_t device,
                                       std::recursive_mutex &hal_lock)
        : hal::hal_device_t(info), hal_lock(hal_lock), device(device) {
    }

The ``device_create`` function needs to call the driver to open the device and
retrieve a device handle. Once we have this handle, a ``refsi_hal_device``
object can be created. If device initialization success, the handle will be
returned:

.. code:: c++

    // hal_main.cpp
    hal::hal_device_t *device_create(uint32_t index) override {
      refsi_locker locker(lock);
      if (!initialized || (index > 0)) {
        return nullptr;
      }
      refsi_device_t device = refsiOpenDevice(REFSI_M);
      if (!device) {
        return nullptr;
      }
      refsi_hal_device *hal_device = new refsi_hal_device(&hal_device_info,
                                                          device, lock);
      if (!hal_device->initialize()) {
        delete hal_device;
        return nullptr;
      }
      return hal_device;
    }

    bool refsi_hal_device::initialize() {
      return true;
    }

The counterpart to ``device_create`` is ``device_delete``, which simply calls
the ``refsi_hal_device`` destructor:

.. code:: c++

    // hal_main.cpp
    bool device_delete(hal::hal_device_t *device) override {
      // No locking - this is done by refsi_hal_device's destructor.
      delete static_cast<refsi_hal_device *>(device);
      return device != nullptr;
    }

Finally, destroying a ``refsi_hal_device`` object should release the device
handle and shut down the device:

.. code:: c++

    // refsi_hal.h
    class refsi_hal_device : public hal::hal_device_t {
     public:
      virtual ~refsi_hal_device();
    };

.. code:: c++

    // refsi_hal.cpp
    refsi_hal_device::~refsi_hal_device() {
      refsi_locker locker(hal_lock);
      refsiShutdownDevice(device);
    }

At this point, running clik examples results in a different error than we have
previously seen:

.. code:: console

    $ bin/copy_buffer
      Using device 'RefSi M1 Tutorial'
      Could not create buffers.
    $ bin/hello
      Using device 'RefSi M1 Tutorial'
      Unable to create a program from the kernel binary.
    $ bin/vector_add
      Using device 'RefSi M1 Tutorial'
      Unable to create a program from the kernel binary.

We can see that the device was created successfully and that the clik example
was able to query its name (``RefSi M1 Tutorial``). The new errors show that
different HAL operations (``mem_alloc`` and ``program_load``) are failing due to
being unimplemented. We will explain how to do so in future sections.
