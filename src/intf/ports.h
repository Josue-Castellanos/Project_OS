#include <stdint.h>
#ifndef SYSTEM_H
#define SYSTEM_H


uint8_t inportb(uint16_t _port);

void outportb(uint16_t _port, uint8_t _data);

#endif