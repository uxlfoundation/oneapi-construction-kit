Supported Toolchains
====================

ComputeAorta is written and compiled using C++11 with some C99. When linking
against LLVM 10 or greater the project will be compiled in C++14 mode.

ComputeAorta is built and tested daily with the following toolchains:

.. rubric:: Supported - tested daily

**Linux**

- x86_64:

  - gcc 7
  - clang 9

- x86:

  - gcc 7

- Arm (cross compiled):

  - gcc 7

- AArch64 (cross compiled):

  - gcc 7

**Windows**

- x86_64:

  - VisualStudio 2017
  - VisualStudio 2019


.. rubric:: Experimental

.. note::

   Any toolchain supporting C++11 (or C++14 when linking against LLVM 10 and
   later) *should* operate as expected.

**Linux**

- x86_64

  - gcc 10
  - gcc 11
  - mingw (gcc 7)
