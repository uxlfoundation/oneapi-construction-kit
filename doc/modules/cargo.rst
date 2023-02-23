************
Cargo Module
************

Cargo is ComputeAorta's STL like container library which conforms
to stricter memory requirements than the C++ STL e.g. constructors
do not allocate memory and exceptions are never thrown from any
container.

CMake Options
#############

.. cmake:variable:: CA_ENABLE_CARGO_INSTRUMENTATION

   Print warning messages about heap allocations and unused SBO
   (Small Buffer Optimization) elements in `small_vector`.
   This is an attempt to provide the client information about
   how to better tune the SBO parameter `N` in the `small_vector`
   template. When cmake:variable:`CA_ENABLE_CARGO_INSTRUMENTATION`
   is enabled cmake:variable:`CA_ENABLE_DEBUG_BACKTRACE` **must**
   also be set.
