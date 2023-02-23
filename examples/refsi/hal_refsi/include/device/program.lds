/* Copyright (C) Codeplay Software Limited. All Rights Reserved. */

OUTPUT_ARCH( "riscv" )

SECTIONS
{

  /*--------------------------------------------------------------------*/
  /* Code and read-only segment                                         */
  /*--------------------------------------------------------------------*/

  /* Begining of code and text segment */
  . = 0x00010000;

  .text :
  {
    *(.text.init)
  }

  /* text: Program code section */
  .text : 
  {
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
  }

  /* rodata: Read-only data */
  .rodata : 
  {
    *(.rdata)
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
  }

  /* End of code and read-only segment */
  . = ALIGN(0x1000);

  /*--------------------------------------------------------------------*/
  /* Initialized data segment                                           */
  /*--------------------------------------------------------------------*/

  /* data: Writable data */
  .data : 
  {
    *(.data)
    *(.data.*)
    *(.srodata*)
    *(.gnu.linkonce.d.*)
    *(.comment)
  }

  /* End of initialized data segment */
  . = ALIGN(16);

  /*--------------------------------------------------------------------*/
  /* Uninitialized data segment                                         */
  /*--------------------------------------------------------------------*/

  /* Start of uninitialized data segment */
  . = .;

  /* sbss: Uninitialized writeable small data section */
  . = .;

  /* bss: Uninitialized writeable data section */
  . = .;
  .bss : 
  {
    *(.bss)
    *(.bss.*)
    *(.sbss*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
  }


  /*--------------------------------------------------------------------*/
  /* Local memory                                                       */
  /*--------------------------------------------------------------------*/

  . = 0x10000000;

  .local :
  {
    *(.local)
  }

  /DISCARD/ : { *(.note.gnu.build-id) }
}
