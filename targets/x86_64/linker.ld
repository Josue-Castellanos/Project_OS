ENTRY(start)

SECTIONS
{
    . = 1M;

    .boot :
    {
        /* multiboot header is the first thing to boot up*/
        KEEP(*(.multiboot_header))
    }

    .text :
    {
        /* Then the text main section */
        *(.text)
    }

    .idt BLOCK(0x1000) : ALIGN(0x1000)
    {
        IDT = .;
        . = . + 0x1000;
    }
}
