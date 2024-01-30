/* Copyright (C) Codeplay Software Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License") with LLVM
 * Exceptions; you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

OUTPUT_ARCH( "riscv" )

MEMORY
{
    mainmem : ORIGIN = 0x10000, LENGTH = (1 << 27)
    localmem : ORIGIN = 0x10000000, LENGTH = 0x200000
}

SECTIONS
{

  /*--------------------------------------------------------------------*/
  /* Code and read-only segment                                         */
  /*--------------------------------------------------------------------*/

  .text : {
    *(.text.init)
  } >mainmem

  /* text: Program code section */
  .text :
  {
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
  } >mainmem

  /* rodata: Read-only data */
  .rodata :
  {
    *(.rdata)
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
  } >mainmem

  /* End of code and read-only segment */

  /*--------------------------------------------------------------------*/
  /* Initialized data segment                                           */
  /*--------------------------------------------------------------------*/

  /* data: Writable data */
  .data ALIGN(0x1000) :
  {
    *(.data)
    *(.data.*)
    *(.srodata*)
    *(.gnu.linkonce.d.*)
    *(.comment)
  } >mainmem

  /* End of initialized data segment */

  /*--------------------------------------------------------------------*/
  /* Uninitialized data segment                                         */
  /*--------------------------------------------------------------------*/

  /* Start of uninitialized data segment */
  . = .;

  /* sbss: Uninitialized writeable small data section */
  . = .;

  /* bss: Uninitialized writeable data section */
  . = .;
  .bss ALIGN(16) :
  {
    *(.bss)
    *(.bss.*)
    *(.sbss*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
  } >mainmem


  /*--------------------------------------------------------------------*/
  /* Local memory                                                       */
  /*--------------------------------------------------------------------*/
  .local :
  {
    *(.local)
  } >localmem

  /DISCARD/ : { *(.note.gnu.build-id) }
}
