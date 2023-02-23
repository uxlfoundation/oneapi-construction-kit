GLSL Module
===========

The GLSL module is used to streamline applications by embedding SPIR-V inside
the executable binary, avoiding the need to load a file from disk at runtime
containing the SPIR-V. This module also tracks depencies between GLSL, SPIR-V,
and the execuable. Liberating developers from having to manually run the
`glslang`_ compiler to generate SPIR-V after every GLSL shader modifications.

.. seealso::
  Embedding SPIR-V in an application is done as part of our ``VectorAddition``
  Vulkan example which uses this CMake module.

  ``UnitVK`` Vulkan unit tests also make uses of ``GLSL.cmake`` to avoid
  having to maintain paths to shader files, allowing the test executable to be
  moved between directories without keeping track of the shader file
  dependencies.

.. _glslang:
  https://github.com/KhronosGroup/glslang

.. cmake-module:: ../../../../source/vk/cmake/GLSL.cmake
