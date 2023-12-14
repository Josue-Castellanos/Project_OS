#pragma once

#include <stdint.h>
#include <stddef.h>


enum {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    BROWN = 6,
    LIGHT_GRAY = 7,
    DARK_GRAY = 8,
    BRIGHT_BLUE = 9,
    BRIGHT_GREEN = 10,
    BRIGHT_CYAN = 11,
    BRIGHT_RED = 12,
    PINK = 13,
    YELLOW = 14,
    WHITE = 15,
};

void print_clear();
void updateCursor();
void print_back_space();
void print_tab();
void int_to_str(uint16_t num, char* buffer, size_t buffer_size);
void print_int(uint16_t num);
void print_char(char character);
void print_str(char* string);
void print_newline();
void print_set_color(uint8_t foreground, uint8_t background);
void reset_key_buffer();
uint16_t strLength(char* string);
uint8_t strEqual(char* string1, char* string2);
void keyboard_handler(void);
void memCpy(void *dest, void *src, size_t count);
void memSet(void *dest, char val, size_t count);
void strncpy(char *dest, char *src, size_t n);
char* strChr(char* str, char ch);






