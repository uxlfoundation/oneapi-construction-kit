Mapping Algorithms To Vector Hardware
=====================================

This task is different from others in this section. Mapping algorithms to
vector hardware using OpenCL and SYCL does not generally involve extending the
API of the execution model. Instead, it involves understanding both the
execution model and hardware capabilities (including parallelism, memory
hierarchies and vector capabilities) and breaking down the compute task in such
a way that it can be efficiently mapped to the hardware's execution units as
well as memory hiearachy. Part of that, such as work-group scheduling, is the
responsiblity of the ComputeMux target. Other parts, such as selecting
appropriate work-group size, N-D range dimensions as well as iteration order,
is the responsibility of the user writing compute kernels.

Work-group Scheduling
---------------------

One of the responsibilities of the ComputeMux runtime is to map kernels to the
hardware in such a way that it takes full advantage of the accelerator's
parallelism and gives the best performance on that hardware. This includes
distributing chunks of kernel computation (work-items and work-groups) between
the different accelerator cores, which we will refer to here as 'work-group
scheduling'.

There are different ways to perform work-group scheduling on the same
accelerator, and there is no single approach that is best for all kinds of
hardware supported by ComputeMux. This is because certain hardware
characteristics such as memory hierarchies, SIMD support and number of cores
will be better suited to one or another approach. We will describe two broad
kinds of approaches here, core-per-work-item and core-per-work-group.

With the core-per-work-item approach, each core in the accelerator is executing
a work-item and a number of these cores are part of a cluster of cores, or
'work-group'. All work-items in a work-group are running simultaneously and
independently with this approach, which means that supporting work-groups larger
than the number of cores in a cluster requires modifying this approach (e.g. by
using OS-level threading). The accelerator typically has several clusters, which
means multiple work-groups can be executed in parallel. Cores in a cluster share
fast on-chip memory that is often not visible to other clusters.

With the core-per-work-group approach, each core in the accelerator is executing
a different work-group. Multiple or even all work-items in the work-group can be
executed simultaneously when `the vecz Whole-Function Vectorizer`_ is used
during compilation. In contrast to the core-per-work-item approach, batches of
work-items execute in lockstep since SIMD instructions are used to achieve full
parallelism from the hardware. Cores do not have to be grouped into clusters and
on-chip memory is not shared between cores but is effectively private to each
core.

Which work-group scheduling approach is the most appropriate typically depends
on both features supported by the hardware as well as the compute kernels that
it executes. When using vecz to take full advantage of the SIMD capabilities of
the accelerator, the 'work-group-per-core' approach is likely to be chosen.
When vecz is not used the memory footprint of the kernel is going to be a
consideration, especially for on-chip :ref:`dedicated shared memory
<overview/example-scenarios/how-to-support-large-scratchpad-memories:Dedicated
Shared Memory>`. This is because choosing 'work-group-per-core' over
'work-item-per-core' increases the per-core footprint of local memory shared
between work-items by a factor of the work-group size. The two approaches are
also going to have very different memory access patterns when data caches are
involved.

Another consideration is the maximum work-group size needed to execute kernels.
It is not uncommon for kernels written with GPUs in mind to use sizes of 256 or
even 512 work-items per work-group. With the 'work-item-per-core' approach, the
maximum work-group size is the total number of accelerator cores in the system.
The 'work-group-per-core' approach does not put a limit to the number of
work-items in a work-group when executing kernels. On the other hand, kernels
written for GPUs may benefit from being tuned for a particular accelerator
architecture in order to achieve the best performance on that accelerator.
Lowering the work-group size and changing the granularity of work done by each
work-item may be effective ways to achieve the desired performance when using
the 'work-item-per-core' approach with such kernels.

A ComputeMux target does not need to limit itself to a single scheduling
approach. It can support multiple approaches and select one based on the kernel
being executed, such as local size, global size, whether vectorization is
enabled, local memory footprint or use of :ref:`dedicated shared memory
<overview/example-scenarios/how-to-support-large-scratchpad-memories:Dedicated
Shared Memory>`.  The Host ComputeMux that executes kernels on the CPU is an
example of a target that implements the 'work-group-per-core' scheduling
approach.

Handling Barriers
-----------------

Work-group barriers are a very important synchronization mechanism in both
OpenCL and SYCL. While hardware cores usually compute a kernel independently of
each other, barriers let the user insert synchronization points in a kernel
function so that cores can wait for a given code region to have been executed by
all cores in a group before continuing the kernel computation. This is often
used for reductions or more generally when cores are sharing 'working' data
using fast on-chip memory.

With the exception of GPUs, most processors are not designed with a heavy focus
on compatibility with compute standards such as OpenCL or SYCL. Instead, support
for such standards is typically added once the architecture is mature. As a
result, very few processors that can fit in the broad 'accelerator' category
have native support for work-group barriers. Without native support, this
feature needs to be implemented using other means, such as existing hardware
features, OS-level support or by using special scheduling and compiler
techniques.

There are two broad categories of approaches that can be used to implement
barriers on accelerator-like processors: synchronization-based and
compiler-based.

The first and most straightforward kind of approach maps work-items to
independent cores and uses synchronization primitives such as mutexes and
condition variables to enable cores to wait until all other cores in the group
have executed the code region that precedes the barrier before continuing
execution. If high-level primitives are not available, barriers can easily be
implemented using atomic instructions such as compare-and-swap (or its
load-linked/store-conditional equivalent), memory fences and waits (to more
efficiently implement spinlocks).

A variant of this approach would be to map work-groups to independent
cores and implement switching between work-items in the work-group with
coroutines (virtual threads) instead of using hardware-based threads. With this
variant, the compiler would compute the maximum stack usage for the kernel so
that the total stack size needed for all coroutines can be calculated.

The second kind of approach is compiler-based, where a compiler pass splits
kernel functions into multiple code regions at barrier boundaries and inserts
scheduling code to handle the execution flow between different regions. This
kind of barrier implementation comes with a small overhead, due to needing to
save per-item state before a barrier and restore it after a barrier. It also
requires 'core-per-workgroup' scheduling to be used. This approach has several
benefits, such as being available to all hardware regardless of supported
features as well as avoiding synchronization between cores.

The compiler-based approach to barriers is typically a requirement for using
the Whole Function Vectorizer to take advantage of the hardware's SIMD
capabilities. For some specific applications it might be possible to turn
barriers into no-ops when the kernel scheduling information and hardware
vectorization capabilities are known when compiling the kernel.

ComputeMux provides an implementation of the compiler-based approach described
above in the form of a 'work-item loops pass' that can be used by any
ComputeMux target. With this pass it is possible to easily execute compute
kernels that make use of barriers without requiring any synchronization
capabilities from the hardware other than what is already needed to execute
barrier-less kernels. The Host ComputeMux target that executes kernels on the
CPU is an example of a target that uses this work-item loops pass to support
kernels with barriers.

The Vecz Whole-Function Vectorizer
----------------------------------

It is common for accelerator cores to feature SIMD instructions that allow
computation to be done on vector values rather than scalar values. For
accelerators that have such vector processing support, fully utilizing these
instructions is very often critical to achieve optimal execution throughput as
the difference in theoretical throughput between vector and scalar instructions
can be a factor of 4 or higher.

However, most AI code is written for GPUs in scalar form with few or no vector
operations. The expectation from the programmer is that the compiler maps the
kernel code to the hardware in such a way that it takes full advantage of the
processor's parallelism and gives the best performance on that hardware. This
might be done by executing the kernel on many SIMD units in lockstep, or by
distributing the kernel computation across separate cores. This model for
writing and executing kernels is called SPMD (Single Program Multiple Data). It
is used by several compute standards such as OpenCL and SYCL.

The purpose of the vecz whole-function vectorizer is to bridge the gap between
the SPMD model used to write kernels and the SIMD ISA exposed by accelerator
cores. vecz can transform SPMD kernel functions into functions that perform the
same computation as the original kernel but on many different inputs at the same
time using vector instructions, keeping all SIMD lanes occupied throughout the
execution of the function.

There are several advantages to this approach compared to taking existing scalar
code and rewriting it by hand to use SIMD instructions for the accelerator:

1. This approach and the SPMD programming model are well-understood by existing
   GPU programmers. As a result there is also a large range of existing
   software that has been written in this way, for example AI models.
2. A single kernel can automatically be made to use up any vector width that is
   supported by the accelerator cores. This makes it much easier to reuse and
   share code that is meant to be run on different accelerator models or
   revisions. It also maximizes the usage of SIMD cores to achieve high
   performance.
3. Since kernel code is translated automatically and in a tailored manner for
   the accelerator cores, it is easy to run the very large range of existing
   software at high performance on new processors.

N-D Range Sizing
----------------

N-D range sizing refers to the selection of a global size and work-group size
(often called the local size) for a particular workload. The global size is
typically tied to the size of the task being performed, i.e. the amount of data
being operated on. In contrast, there will often be a wide range of valid
work-group sizes.

Selection of an appropriate N-D range size is an important factor in achieving
optimal performance, and one which is generally up to the end-user to decide.
There is often an overhead associated with a large number of small groups, but a
limit on the size of the work-groups when using local memory. A ComputeMux
implementation **must** provide a default local size, in case the user does not
specify one. 

A ComputeMux implementation **must** provide a maximum work-group size, as well
as a maximum size for each of the three dimensions.

.. tip::
   Because selection of the size is a runtime feature, it is generally easy to
   benchmark a number of sizes to find the most optimal one - this can even be
   done programmatically.

Kernels may contain local memory. Local memory is shared by work-items in a
work-group. It can be used by kernels to share information between work-items,
when used in conjunction with barriers to perform synchronization. In this
scenario, the amount of local memory a kernel uses will scale with the size of
the work-group. The larger the work-group, the more local memory is used.
Because local memory is usually implemented by a ComputeMux target in 'fast'
memory (sometimes called TCM (tightly-coupled memory), which may be a small
amount of per-core SRAM), there is a natural limit to the size of work-groups.
Smaller work-groups will use less memory but may be more affected by
synchronization overhead. The exact performance characteristics are determined
by the algorithmic nature of the kernel as well as how the ComputeMux
implementation maps these features onto hardware.

In addition to local memory footprint, another consideration when selecting N-D
range dimensions is which 'coarseness' of parallelism is most appropriate for
the given hardware. With 'fine' parallelism, a N-D range would contain many
work-items, each of which performs a small amount of work (e.g. computing a
single pixel in an image). With 'coarse' parallelism, a N-D range would instead
contain a smaller number of work-items, each of which performs a larger amount
of work (e.g. computing a :math:`N \times N` block of an image, part of a row or
even a whole row of an image). Overheads inherent in switching between
work-items, as well as a smaller number of cores, might favor coarser
parallelism compared to the fine parallelism approached used with GPUs. However,
fine parallelism may be favored when using the vecz whole-function vectorizer.
This is because scalar operations can more effectively be vectorized than
existing vector operations or loops.

The work-group size also impacts on the ability to use vectorization. When
kernels are compiled, Vecz will vectorize along a single dimension. This means
that the generated SIMD instructions will group work-items along one of the
three dimensions. Typically this is the :math:`x` dimension (which is the
inner-most dimension). This means the work-group size chosen for a vectorized
kernel should be a minimum of the vectorization factor along the vectorization
dimension. For example, a kernel vectorized along the :math:`x` dimension by a
factor of 8 will run with a work-group size of 16x4x1, but not with a work-group
size of 4x8x2. In practice, it is unusual to encounter work-group sizes that are
too small to allow vectorization to be used when re-using existing kernels.

The end-user is able to manually specify a vectorization factor at compile-time.
This can be used when a smaller work-group size is expected, or when it is known
to help performance.

Typically a ComputeMux implementation will preserve a scalar version of the
kernel, so if the work-group size prevents execution with vectorization, it can
fall back to the scalar version at runtime.
