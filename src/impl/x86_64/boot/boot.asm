global start    ; start can be accessed outside this file
extern long_mode_start  ; extern is a file outside of boot.asm that can be accessed

section .text   ; This is a section directive that indicates 
                ; the beginning of the "text" section that contains
                ; executable code

bits 32         ; 32-bit data x86-architecture   

start:
    ; First thing is to create a stack
    ; The CPU uses the ESP Register to determine the address of the current stack 
    ; frame. We will store the address of the top of the stack into this register 
    ; since there are no frames on the stack at boot.
    mov esp, stack_top      ; stack pointer

    ; In order to avoid bugs and errors with old and outdated CPUs we need to check
    ; if the CPU supports every needed feature to run the kernel/OS.
    call check_for_multiboot    ; checks if the kernel has been uploaded by a multiboot2 loader
    call check_for_cpuid        ; checks if CPUID is supported by CPU
    call test_for_long_mode     ; If so, will use CPUID to check for long mode support

    ; Entering Long Mode
    call setup_page_tables      ;
    call enable_paging          ;

    ; Set up GDT
    lgdt [gdt64.pointer]   ; Load the GDTR with the address of the GDT descriptor
    jmp gdt64.code:long_mode_start  ;

    hlt

check_for_multiboot:
    ; The CPU is currently in protect mode and only allows 32-bit instructions
    ; When the boot loader invokes the 32-bit operating system, the 
    ; machine must contain the magic value '0x36d76289' in the EAX registery.
    ; The bootloader must write the magic value to it before loading a kernel.
    ; Having this magic value lets the operating system know that it was loaded 
    ; by a Multiboot-compliant boot loader.

    cmp eax, 0x36d76289 ; cmp instruction compares the value in EAX to the magic value
    jne .no_multiboot   ; If EAX does not contain the magic value, jump to process error
    ret                 ; otherwise we will return to the routine

.no_multiboot:
    ; No multiboot2 loader
    mov al, "M"     ; set the error code to "M" in the al register for multiboot failure
    jmp error       ; jump to the error function

check_for_cpuid:
    ; Prior to using the CPUID instruction, we need to make sure the CPU supports it by 
    ; testing the 'ID' bit #21 (0x00200000) in the EFLAGS register. If we can flip the ID 
    ; bit form '0' to '1' then the CPUID is available in the CPU. We'll start by copying the
    ; current 32-bit contents of EFLAGS register by pushing the entire EFLAGS register onto the stack.
    
    ; Set the ID bit in EFLAGS, bits 0-15 are 0x0000, bits 16-31 are 0x0000 0000, bits 
    pushfd          ; PUSHFD instruction again saves the current state of the EFLAGS register on the stack 
                    ; allowing you to modify the flags and later restore them to original state.
    pop eax         ; This will pop the EFLAGS from the stack and load them into the EAX register.
    mov ebx, eax
    xor eax, 1 << 21; Flips the ID bit (bit 21) from '0' to '1' in 0x00200000
    push eax        ; Push the modified EFLAGS back onto the top of the stack.
    popfd           ; Pop EFLAGS with ID bit #21 flipped back into EFLAGS register.

    ; Check if the ID bit is still set
    pushfd          ; Push the modified EFLAGS back onto the stack
    pop eax         ; Pop the modified EFLAGS from the stack back into the EAX register.
    push ebx
    popfd
    cmp eax, ebx    ; Compare EAX to (1 << 21), if they are equal that means it wasn't flipped
                    ; so we know that the CPU supports CPUID.
            ; Restore the saved original EFLAGS from the stack back into EFLAGS register
    je .no_cpuid    ; If the comparison fails then the CPU does not support CPUID and jump to process error
    ret             ; Otherwise we will return to the routine

.no_cpuid:
    ; CPUID not supported
    mov al, "C"     ; set the error code to "C" in the al register for CPUID failure
    jmp error       ; jump to the error function

test_for_long_mode:
    ; Now that CPUID is available we have to check whether long mode can be used or not. 
    ; Long mode can only be detected using the extended functions of CPUID (> 0x80000000),
    ; so we have to check if the function that determines whether long mode is available or 
    ; not is actually available. The CPUID instruction provides a lot of information about 
    ; the capabilities of the CPU, but it doesn't explicitly indicate whether the CPU supports 
    ; 64-bit mode (Long Mode).

    ; Test if extended processor info is available
    mov eax, 0x80000000    ; Set EAX register with magic value 0x80000000, implicit argument for CPUID
    cpuid                  ; Request the highest extended feature information from CPUID (> 0x80000000)
                           ; The results are stored in various registers including EAX, EBX, ECX, and EDX
                           ; To determine the highest extended function calling parameter, call CPUID with EAX = 80000000
    cmp eax, 0x80000001    ; If it’s at least 0x80000001, we can test for long mode as described above. 
    jb .no_long_mode       ; If it is below, CPU is too old for long mode and jump to process error

    ; Use extended info to test if Long Mode is available
    mov eax, 0x80000001    ; Set the EAX register to 0x80000001.
    cpuid                  ; To test if long mode is available, we need to call CPUID with 0x80000001 in EAX
    test edx, 1 << 29      ; The test instruction checks if the LM-bit which is bit 29 in the EDX register is set to '1'.
    jz .no_long_mode       ; If the flag is set to zero, there is no long mode available, jump to process error
    ret                    ; Else, return to routine if Long mode is supported

.no_long_mode:
    ; Long mode not supported by CPU
    mov al, "L"        ; set the error code to "L" in the al register for Long Mode failure
    jmp error           ; jump to the error function

;PAGING TABLE
setup_page_tables:
    ; We will do Identity mapping where we map a physical address to the exact same virtual
    ; address, this will be activated as soon as we enable long mode. The CPU can assume that 
    ; the first 12 bits of all the addresses are zero. If they're always implicitly zero, we can 
    ; use them to store metadata without changing the address.

    ; Point the first entry of the level 4 page table to the first entry in the Level 3 table.
    mov eax, p3_table   ; Copies the contents of the first third-level page table entry into the EAX register.
    or eax, 0b11        ; When we OR with 0b11, it means that the first two bits will be set to one, leaving the rest as they were.
                        ; The first two bits are the ‘present bit’ and the ‘writable bit’. By setting the first bit,
                        ; we say “this page is currently in memory,” and by setting the second, we say “this page is 
                        ; allowed to be written to.”
    mov [p4_table], eax; EAX has P3 table address and move it to the first 4 bytes of the P4 table.

    ; Point the first entry of the level 2 page table to the first entry in the Level 3 table.
    mov eax, p2_table  ; Copies the contents of the first second-level page table entry into the EAX register.
    or eax, 0b11        ; present + writable
    mov [p3_table], eax; EAX has P2 table address and move it to the first 4 bytes of the P3 table.

    ; Point each Level 2 page table entry to a page. In order to get the right memory location, we will multiply
    ; the number of the loop counter by 0x200000:
    mov ecx, 0      ; counter for our loop

.map_p2_table_loop:
    ; Now we need to map P2’s first entry to a huge page starting at 0, P2’s second entry to a huge page 
    ; starting at 2MiB, P2’s third entry to a huge page starting at 4MiB, and so on.

    ; Map ecx-th Level 2 page tables entry to a huge page that start at address 2 MiB * ecx
    mov eax, 0x200000  ; 2MiB becasue each page is two megabytes in size.
    mul ecx
    or eax, 0b10000011  ; Here, we don’t just OR 0b11: we’re also setting another bit. 
                        ; This extra 1 at the end is a ‘huge page’ bit, meaning that the pages are 
                        ; 2,097,152 bytes. Without this bit, we’d have 4KiB pages instead of 2MiB pages.
    mov [p2_table + ecx * 8], eax ; We are now mapping the value in EAX to the ecx-th entry multiplied by
                                  ; 8 becasue each entry is 8 bytes large in 2MiB page size. 

    ; Bookkeeping
    inc ecx         ; Increment counter by 1
    cmp ecx, 512    ; Checks if counter = 512, if so the whole Level 2 page table is mapped.
                    ; The page table is 4096 bytes, each entry is 8 bytes, so that means there are 512 entries. 
                    ; This will give us 512 * 2 mebibytes: one gibibyte of memory.
    jne .map_p2_table_loop  ; Jump to map the next entry if the page does not have 512 entries.

    ; Now the first gigabyte (512 * 2MiB) of our kernel is identity mapped and in turn 
    ; accessible through the same physical and virtual addresses.
    ret             ; Return to routine

enable_paging:
    ; Once paging is enabled CPU can enter Long Mode and in turn access 64bit instructions and registers.
    ; We have to pass the address of the level 4 page table to the CPU. The CPU looks for this address in
    ; a special control register, the CR3 register to access it.
    mov eax, p4_table   ; Because it’s a special register, it has some restrictions and one of those is 
                        ; that when you mov to cr3, it has to be from another register.
    mov cr3, eax        

    ; Enabling physical address extention (PAE) CR4 register
    mov eax, cr4        ; Set the control 4 register to the EAX register
    or eax, 1 << 5      ; Set the PAE-bit, in CR4 which is the 6th bit (bit #5)
    mov cr4, eax        ; Set the EAX register back to the control register 4

    ; Enabling long mode EFER MSR register
    mov ecx, 0xC0000080; Set the ECX register to the long mode bit in the EFER MSR register address which is 0xC0000080
    rdmsr               ; read from a model specific register
    or eax, 1 << 8      ; Set the Long Mode (LM) bit which is the 9th bit (bit #8) in EFER MSR register
    wrmsr               ; write to a model specific register

    ; Enabling paging and system call extention in CR0 register
    mov eax, cr0        ; Set the EAX register to control register 0
    or eax, 1 << 31     ; Set the Paging (PG) bit which is the 31st bit
    or eax, 1 << 16     ; Set the Write Protect (WP) bit.  This is a security feature to prevent accidental
                        ; or unauthorized writes to protected memory regions.
    mov cr0, eax        ; Set control register 0 to the EAX register

    ret
error: 
    ; This label is an error procedure that handles errors if the CPU does not 
    ; support every needed feature, if not the kernel aborts and prints an error message.

    ; prints "ERR: $" where $ is the error code in al register.
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov byte  [0xb800a], al     ; ascii letter error code in the al register
    hlt     ; after the error code is printed halt the CPU.

section .bss    ; The BSS section is used to declare uninitialized data. 
align 4096      ; Set up paging tables, except Level 1: Page Table (PT)......
    ; Paging is a memory management scheme that separates virtual and physical memory. 
    ; The address space is split into equal sized pages and a page table specifies which 
    ; virtual page points to which physical page. In protected mode a page table entry was 
    ; only four bytes long, so you had 1024 entries per table. However, In long mode you 
    ; only have 512 entries per table as each entry is eight bytes long. This means that 
    ; it uses a page size of 4096 bytes and a 4 level page table.
p4_table:       ; Page-Map Level-4 Table (PML4)
    resb 4096
p3_table:       ; Page-Directory Pointer Table (PDP)
    resb 4096
p2_table:       ; Page-Directory Table (PD)
    resb 4096

;STACK
stack_bottom:   ; Set up stack  
    ; The stack will contain statically allocated variables.In this context, 
    ; it's used to allocate space for the stack. On many x86 architectures, the 
    ; stack needs to be aligned to a 4-byte boundary. By allocating 4KB (4096 bytes)
    ; of stack space, you ensure that the stack pointer (SP) is properly aligned with 
    ; suffecient memory.

    resb 4096 * 4  ; The resb (reserve bytes) directive is used to reserve a block of 
                ; memory without initializing its contents
stack_top:

section .rodata
; The processor is still in a 32-bit compatibility submode. To actually execute 64-bit code,
; we need to set up a new Global Descriptor Table.GRUB has set up a valid 32-bit GDT  
; but now we need to switch to a long mode GDT.
gdt64:
    ; We’ll use this label later, to tell the hardware where our GDT is located.
    ; The GDT will have three entries zero entry, code segment, and data segment but in 64-bit mode
    ; data segment is not necessary
    dq 0             ; Zero entry. The first entry in the GDT needs to be a zero value.
.code: equ $ - gdt64
    ; Code segments descriptor have the following bits set: 
    ; executable (43), descriptor type (44), present (47), and 64-bit flag (53). 
    ; Base: 0x0, limit: 0xFFFF,
    ; 1st flags: (Present)1 (Privilege)00 (Desctiptor type)1 -> 1001b
    ; Type flags: (code)1 (Conforming)0 (Readable)1 (Accessed)0 -> 1010b
    ; 2nd flags: (Granularity)1 (32-bit default)1 (64-bit seg)0 (AVL)0 -> 1100b
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
    dw $ - gdt64 - 1    ; size of our GDT, its always less than one of our true size
                        ; size = .pointer - gdt64 - 1
    dq gdt64            ; start address of our gdt64

