#ifndef STR_MANIP_H
#define STR_MANIP_H
#include <stdint.h>
#include <stddef.h>


// Length of string
uint16_t strLength(char* string);

// Checks two strings
uint8_t strEqual(char* string1, char* string2);

// Copy's two strings
void strncpy(char* dest, char* src, size_t n);

// Checks character in string
char* strChr(char* str, char ch);

// Converts integer to string for print
void int_to_str(uint16_t num, char* buffer, size_t buffer_size);

#endif