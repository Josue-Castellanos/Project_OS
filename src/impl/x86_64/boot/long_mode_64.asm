global long_mode_start
;global create_file
; global write_file
; global delete_file
; global rename_file
; global open_file
; global change_dir

section .text
bits 64

extern IDT
extern kernel_main
extern keyboard_handler

idt_common_handler:
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    pushfq

    call keyboard_handler   ; IRQ1 handler

    popfq
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax

    ret
irq1:
    call idt_common_handler
    iretq
    GLOBAL irq1
idt_descriptor:
    dw 4095
    dq IDT
load_IDT:
    lidt [idt_descriptor]    ; Load IDTR
    sti
    ret
    GLOBAL load_IDT

long_mode_start:
    mov ax, 0           ; Set the A-register to the data descriptor.
    mov ss, ax          ; Set the data segment to the A-register.
    mov ds, ax          ; Set the extra segment to the A-register.
    mov es, ax          ; Set the F-segment to the A-register.
    mov fs, ax          ; Set the G-segment to the A-register.
    mov gs, ax          ; Set the stack segment to the A-register.
    
    call kernel_main
    
    hlt
