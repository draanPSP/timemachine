OUTPUT_FORMAT("elf32-littlemips", "elf32-bigmips", "elf32-littlemips")
OUTPUT_ARCH(mips:allegrex)
ENTRY(_start)

SECTIONS
{
    . = 0x040c0000;
    .text :
    {
        *(.text.vector)
        *(.text)
    }
    .sdata : { *(.sdata) *(.sdata.*) *(.gnu.linkonce.s.*) }
    .rodata : { *(.rodata) }
    .data : { *(.data*) }
    .bss :
    {
        __bss_start = .;
        *(.bss*)
        __bss_end = .;
    }
}
