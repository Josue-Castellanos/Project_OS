; The layout of Multiboot2 header*
; magic = 0xe85250d6 
; architecture = 0 protected mode
; header_length = size of this multiboot header 
; checksum = 512 bytes, making sure
; tags  = required tags for 0,0,8 

section .multiboot_header
header_start:
    ;magic number 
    dd 0xe85250d6    ; The loader searches for a magic number to find the 
                     ; header in the boot sector which is 0xE85250D6 bytes for Multiboot2
    ;dd 0x10000       ; Address where the boot sector will be loaded

    ;architecture
    dd 0    ;0 = 32-bit protected mode of i386 cpu
            ;4 = 32-bit MIPS64
    ;header length
    dd header_end - header_start    ;header length specifies the length of Multiboot2 header in bytes including tags dd, dw
    ;checksum calculation
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))  

    ;required end tags
    dw 0    ;type
    dw 0    ;flags
    dd 8    ;size
header_end:

