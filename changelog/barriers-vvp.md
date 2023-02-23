Feature additions:

 * If the `HandleBarriersPass` finds a vector kernel and a vector-predicated
   kernel with the same source, they are combined into one wrapper. The
   vector-predicated kernel will not generate its own entry point, even if it
   is marked as an entry point.
