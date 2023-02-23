# Vecz Control Flow Conversion Switch failure:

Bug fixes:
* Fix crash in Control Flow Conversion caused by presence of Switch
  instructions, which are not handled.
* The command line tool `veczc` now has a command line option that allows
  vectorization to fail without returning an error state, for testing failures.


