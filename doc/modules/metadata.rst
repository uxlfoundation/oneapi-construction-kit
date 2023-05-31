Metadata API
============

This is a library that can be used by applications to read and write metadata to and from 
a well-specified binary format. This is mainly used in the oneAPI Construction Kit to store kernel and
runtime metadata for offline compiled kernels.

Design
------

Metadata is added by pushing values to a `stack`. Each stack must be registered to the metadata 
context with a unique name. When the context is finalized, all stack will be serialized into the binary format.
A stack can accept a value of any supported type, and supports arbitrary nesting of structures of values in arrays
or hash-tables. 

Stack data is serialized using one of two formats: ``MD_RAW_BYTES`` & ``MD_MSG_PACK``.
``MD_RAW_BYTES`` will recursively serialize each value on the stack into an endian-appropriate byte encoding and 
write it to the output stream. It will therefore not preserve any type or length knowledge of the stack
and will simply be interpreted as a single byte-array when loaded. It is then the callers responsibility to reinterpret 
the raw bytes into the data they expect. For simple metadata this may be sufficient and will reduce the overall
size of the binary, but for more complex metadata structures, especially those that use variable-length arrays, we 
recommend using ``MD_MSG_PACK`` format, which includes additional type and length information when serialized
and will restore the stack exactly back to its pre-serialized state.

Hooks
-----

The metadata API can be used in a variety of contexts and therefore its behaviour
depends on user-provide callback functions, which we refer to as `hooks`. These hooks
inform the API where to read and write metadata and optionally provide a custom implementation
for allocation and de-allocation, if using a custom allocator.

.. code:: c

    struct md_hooks {
        void *(*map)(void *userdata, size_t *n);
        md_err (*write)(void *userdata, const void *src, size_t n);
        void (*finalize)(void *userdata);
        void *(*allocate)(size_t size, size_t align, void *userdata);
        void (*deallocate)(void *ptr, void *userdata);
    };

-  ``map`` - get a pointer to the start of a previously serialized metadata binary. The length
   of the binary is stored into the pointer ``n``.
-  ``write`` - write ``n`` bytes from ``src`` to somewhere.
-  ``finalize`` - perform any cleanup operations i.e. close the file handle.
-  ``allocate`` - allocate ``n`` bytes of memory.
-  ``deallocate`` de-allocate a pointer previously allocated with ``allocate``.


Basic Usage
-----------

The metadata API can effectively be used in 2-modes: reading metadata from an existing 
metadata binary; or creating metadata and writing it out to a new binary.

Reading from an existing binary
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Construct an ``md_hooks`` struct with the required ``map()`` callback as to where to read the data from.
2. Initialize a metadata context with ``md_init()``. Since the map hook was provided this step will attempt to
   deserialize the binary. If it fails an error will be returned.
3. Get a handle to a stack using ``md_get_block()``. If no stack with that name exists ``nullptr`` is returned.
4. Read the values from the stack using the appropriate calls based on the data e.g. ``md_get_uint()`` 
   or ``md_get_zstr()``.
5. When finished, release the context with ``md_release_ctx()``, this will destroy and de-allocate the context.


Writing metadata to a binary
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Construct an ``md_hooks`` struct with the required ``write()`` and ``finalize()`` hooks.
2. Initialize an empty metadata context with ``md_init()``.
3. Register a new stack with a unique name to the context with ``md_create_block()``.
4. Push values to the stack using the appropriate type-compatible function e.g. push an unsigned integer with
   ``md_push_uint()``.
5. Once you have pushed all values to the stack, finalize the stack with ``md_finalize_block()``.
6. Finalize the context with ``md_finalize_ctx``.
7. Release and de-allocate the context with ``md_release_ctx()``.





