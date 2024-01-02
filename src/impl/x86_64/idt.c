#include "constants.h"
#include "idt.h"
#include "ports.h"

extern void load_IDT(void);


void idt_init(void) {
  extern struct IDT_entry IDT[IDT_SIZE];
  extern void irq1();
  // ISR - The addresses of individual Interrupt Service Routines (ISRs) for
  // hardware interrupts (IRQs) are declared. These addresses correspond to functions
  // like irq0, irq1, and so on, which are responsible for handling specific interrupts.

  // Fill the IDT descriptor
  uint64_t irq_address = (uint64_t)irq1;
    uint8_t IRQ1 = 33; 
    IDT[IRQ1].offset_low = (uint16_t)(irq_address & 0xFFFF);
    IDT[IRQ1].selector = KERNEL_CODE_SEGMENT_OFFSET; // KERNEL_CODE_SEGMENT_OFFSET
    IDT[IRQ1].ist = 0;
    IDT[IRQ1].type_attr = INTERRUPT_GATE; // 64-bit INTERRUPT_GATE
    IDT[IRQ1].offset_mid = (uint16_t)((irq_address & 0xFFFF0000) >> 16);
    IDT[IRQ1].offset_high = (uint32_t)((irq_address & 0xFFFFFFFF00000000) >> 32);
    IDT[IRQ1].zero = 0;

    // Initiating PIC
    remapPIC();

    // Unmask IRQ1 -> 0xFD = 1111 1101 in binary enables only IRQ1
    outportb(PIC_MASTER_DATA, 0xFD);  // Mask all other interupts

    load_IDT();
}

void remapPIC(void) {
	uint8_t a1, a2;
 
	a1 = inportb(PIC_MASTER_DATA);       // save masks
	a2 = inportb(PIC_SLAVE_DATA);
 
    // ICW1 - Initialize
    outportb(PIC_MASTER_CMD, ICW1_ICW4_INIT);  // Start initialization sequence (ICW1)
    outportb(PIC_SLAVE_CMD, ICW1_ICW4_INIT);

    // ICW2 - Set the base interrupt vectors
    outportb(PIC_MASTER_DATA, 0x20);  // Offset for master PIC (IRQs 0-7)
                                      // Place holder starting at 32
    outportb(PIC_SLAVE_DATA, 0x28);   // Offset for slave PIC (IRQs 8-15)
                                      // Place holder starting at 40
    // ICW3 - Configure cascading (master to slave and slave to master)
    outportb(PIC_MASTER_DATA, ICW1_INTERVAL4);  // Tell master PIC that there is a slave at IRQ2
    outportb(PIC_SLAVE_DATA, ICW1_SINGLE);   // Tell slave PIC its cascade identity

    // ICW4 - Additional configuration
    outportb(PIC_MASTER_DATA, ICW4_8086);  // Set master PIC to 8086/88 (MCS-80/85) mode
    outportb(PIC_SLAVE_DATA, ICW4_8086);   // Set slave PIC to 8086/88 (MCS-80/85) mode

    outportb(PIC_MASTER_DATA, a1);   // restore saved masks.
	outportb(PIC_SLAVE_DATA, a2);

    // Masking all bits in bot PICs
    outportb(PIC_MASTER_DATA, MASK_ALL_INTS);  // Mask all interrupts 
    outportb(PIC_SLAVE_DATA, MASK_ALL_INTS);   // Mask all interrupts 
}