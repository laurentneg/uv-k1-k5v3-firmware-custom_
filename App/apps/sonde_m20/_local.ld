APP_VMA = 0x20000280;
APP_LENGTH = 0x1000;
ENTRY(app_main)
MEMORY { APP (rwx) : ORIGIN = 0x20000280, LENGTH = 0x1000 }
SECTIONS {
  .app 0x20000280 : {
    KEEP(*(.text.entry))
    *(.text .text.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
    *(.data .data.*)
    . = ALIGN(4);
    __app_bss_start = .;
    *(.bss .bss.* COMMON)
    . = ALIGN(4);
    __app_bss_end = .;
  } > APP
  __app_end = .;
  ASSERT(__app_end <= 0x20001280, "overflow")
  /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) *(.comment) *(.note.*) }
}
