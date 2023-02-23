Allocating and Copying Device Memory
------------------------------------

Now that we are using the RefSi driver to open a connection to a device, more
useful HAL operations can be implemented, such as allocating and freeing memory,
as well as copying data between host and device memory.

The most straightforward of these operations are allocating and freeing device
memory. This is because there is almost a 1:1 mapping between driver functions
and HAL operations. These functions operate like ``malloc`` and ``free``,
except that the allocated memory lives on the device:

.. code:: c++

    // refsi_hal.cpp
    hal::hal_addr_t refsi_hal_device::mem_alloc(hal::hal_size_t size,
                                                hal::hal_size_t alignment) {
      refsi_locker locker(hal_lock);
      return refsiAllocDeviceMemory(device, size, alignment, DRAM);
    }

    bool refsi_hal_device::mem_free(hal::hal_addr_t addr) {
      refsi_locker locker(hal_lock);
      return refsiFreeDeviceMemory(device, addr) == refsi_success;
    }

Implementing memory transfer operations is slightly more complicated. It
involves mapping the address in device memory to an address (pointer) which can
be used directly by the host. Once this is done, ``memcpy`` can be used to copy
data between host and device:

.. code:: c++

    // refsi_hal.cpp
    #include <string.h> // Added
    ...
    bool refsi_hal_device::mem_read(void *dst, hal::hal_addr_t src,
                                    hal::hal_size_t size) {
      refsi_locker locker(hal_lock);
      void *src_mem = refsiGetMappedAddress(device, src, size);
      if (!src_mem) {
        return false;
      }
      memcpy(dst, src_mem, size);
      return true;
    }

    bool refsi_hal_device::mem_write(hal::hal_addr_t dst, const void *src,
                                     hal::hal_size_t size) {
      refsi_locker locker(hal_lock);
      void *dst_mem = refsiGetMappedAddress(device, dst, size);
      if (!dst_mem) {
        return false;
      }
      memcpy(dst_mem, src, size);
      return true;
    }

As previously mentioned, ``hal_addr_t`` hold addresses that are opaque to the
host CPU and cannot be cast to a pointer for dereferencing. What
``refsiGetMappedAddress`` does is return a pointer to an area of memory which is
accessible to the CPU and where memory accesses are mirrored on the
corresponding area of device memory. This means writes by the CPU are
automatically seen by the device when it reads the memory, and similarly the CPU
will observe changes made by the device when it reads from the mapped memory
area.


.. note::

    The RefSi platform works in such a way that the entire device DRAM is always
    mapped in the host CPU's address space. As a result, the user of the RefSi
    driver (e.g. the RefSi HAL) does not need to manually unmap memory regions.

At this point, running ``copy_buffer`` results in the example finishing
successfully:

.. code:: console

    $ bin/copy_buffer
    Using device 'RefSi M1 Tutorial'
    Results validated successfully.

Running the ``hello`` and ``vector_add`` examples results in the same error we
have seen previously. We will look at how to address this issue in the next
section.

.. code:: console

    $ bin/hello
    Using device 'RefSi M1 Tutorial'
    Running hello example (Global size: 8, local size: 1)
    Unable to create a program from the kernel binary.

    $ bin/vector_add
    Using device 'RefSi M1 Tutorial'
    Running vector_add example (Global size: 1024, local size: 16)
    Unable to create a program from the kernel binary.
