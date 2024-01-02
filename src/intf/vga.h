#ifndef VGA_H
#define VGA_H
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

const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;
const static size_t TAB_SIZE = 8;

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

#endif








