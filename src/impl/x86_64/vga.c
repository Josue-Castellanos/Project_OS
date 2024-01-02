#include "vga.h"
#include "ports.h"
#include "constants.h"
#include "str_manip.h"
#include "common.h"

struct Char
{
    uint8_t character;
    uint8_t color;
};

char* clear_buffer;
uint8_t key_buffer_index = 0;

// VGA memory buffer
struct Char* buffer = (struct Char *)0xb8000;
uint8_t color = GREEN | BLACK << 4;

// Updates Cursor
void updateCursor() {
    uint16_t temp;

    temp = row * NUM_COLS + col;

    outportb(REG_SCREEN_CTRL, 14);
    outportb(REG_SCREEN_DATA, temp >> 8);
    outportb(REG_SCREEN_CTRL, 15);
    outportb(REG_SCREEN_DATA, temp & 0xFF);
}

// Clears all the columns in a given row starting at 0xb80000
void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
    updateCursor();
}

// Clears screen
void print_clear() {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
    row = 0;
    col = 0;
    updateCursor();
}

// print new line
void print_newline() {
    col = 0;
    if (row < NUM_ROWS - 1) {
        row++;
        return;
    }

    for (size_t row = 1; row < NUM_ROWS; row++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * row];
            buffer[col + NUM_COLS * (row - 1)] = character;
        }
    }
    clear_row(NUM_COLS - 1);
}

// print characters
void print_char(char character) {
    if (character == '\n') {
        print_newline();
        reset_key_buffer(key_buffer);
        return;
    }
    if (character == '\b') {
        print_back_space();
        return;
    }
    if (character == '\t') {
        print_tab();
        return;
    }

    if (col > NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    key_buffer[key_buffer_index] = (uint8_t) character;
    key_buffer_index++;

    col++;
    updateCursor();
}

// print string
void print_str(char* string) {
   
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) string[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

// function to print an integer
void print_int(uint16_t num) {
    // Convert the integer to a string for printing
    char num_str[12]; // Maximum of 11 digits for an integer (including sign)
    int_to_str(num, num_str, sizeof(num_str));

    // Print the string
    print_str(num_str);
}

//  Print color for text and background
void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground | (background << 4);
}

// Backspace
void print_back_space()
{
    if (col > 8)
    {
        col--;
        struct Char empty = {
            .character = ' ',
            .color = color,
        };
        buffer[col + NUM_COLS * row] = empty;
        updateCursor();
    }
}

// Tab
void print_tab()
{
    col += TAB_SIZE;
    updateCursor();
}

void reset_key_buffer() {
    key_buffer = '\0';
    // Reset the buffer index
    key_buffer_index = 0;
}








