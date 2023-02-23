# Abacus Payne Hanek UB fix:

Bug fixes:
* Fixed undefined behaviour in the implementation of trigonometric builtins that
  only manifested in rare edge cases (when invoked with a constant argument).
