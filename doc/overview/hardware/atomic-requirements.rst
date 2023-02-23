Atomic Requirements
===================

This section details advanced memory requirements that a Mux target **may**
support in hardware. Hardware features listed here are considered *good to
haves* and can be utilized by a Mux implementation. Importantly the concepts
and features these requirements map to in Mux **may** be implemented in software
and hardware support is truly optional for the target.

Atomics and Fences
------------------

A Mux target **may** use hardware atomics and fences to implement the set of
instructions defined in the :ref:`Atomics and Fences
<specifications/mux-compiler-spec:Atomics and Fences>` section of the compiler
specification.

An atomic operation that reads, modifies or writes memory **must** do so in one
indivisible operation such that from the perspective of an executing thread in
a multi-threaded context the operation has either not happened or happened.
The scope of hardware atomic operations **may** be restricted to threads within
the same core, in which case cross core atomic operations **should** be
simulated in software using synchronization primitives.

Fence operations enforce ordering constraints on memory accesses restricting
the instruction reordering optimizations that can be made by a compiler or a
instruction scheduling unit on an out-of-order execution processor. Fence
operations **may** also maintain cache coherence between threads which have
access to the same memory, but use different caches. A processor without fence
instructions in its ISA **may** provide a software implementation of the fence
(i.e. a compiler fence).
