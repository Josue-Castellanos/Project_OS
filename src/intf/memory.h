#ifndef MEMORY_H
#define MEMORY_H
#include <stddef.h>

//
void memCpy(void *dest, void *src, size_t count);

//
void memSet(void *dest, char value, size_t count);

#endif