#include <stdint.h>
#include "ports.h"

// The general-purpose registers are 64-bit, so you need to use 
// the 32-bit lower portion of registers (e.g., EAX, EDX) for "in" and "out".
uint8_t inportb(uint16_t _port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a" (result) : "dN" (_port));
    return result;
}

void outportb(uint16_t _port, uint8_t _data) {
    asm volatile("outb %0, %1" : : "a" (_data), "dN" (_port));
}