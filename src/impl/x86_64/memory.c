#include "memory.h"

void memCpy(void *dest, void *src, size_t count)
{
    char *dest_ptr = (char *)dest;
    char *src_ptr = (char *)src;
    for (size_t i = 0; i < count; ++i)
    {
        dest_ptr[i] = src_ptr[i];
    }
}

void memSet(void *dest, char value, size_t count)
{
    char *dest_ptr = (char *)dest;
    for (size_t i = 0; i < count; ++i)
    {
        dest_ptr[i] = value;
    }
}

// malloc