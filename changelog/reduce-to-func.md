Bug fixes:

* The `ReduceToFunctionPass` was fixed in the presence of multiple vectorized
  forms of a kernel. It would previously crash if a base function was linked to
  more than one vectorized form.
