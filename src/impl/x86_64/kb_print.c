#include "kb_print.h"
#include "ports.h"
#include "constants.h"
#include <stdint.h>

char* clear_buffer;
char* key_buffer;
uint8_t key_buffer_index = 0;

const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;
const static size_t TAB_SIZE = 8;

struct Char
{
    uint8_t character;
    uint8_t color;
};

// VGA memory buffer
struct Char* buffer = (struct Char *)0xb8000;
size_t col = 0;
size_t row = 0;
uint8_t color = GREEN | BLUE << 4;

// Cursor
void updateCursor() {
    uint16_t temp;

    temp = row * NUM_COLS + col;

    outportb(REG_SCREEN_CTRL, 14);
    outportb(REG_SCREEN_DATA, temp >> 8);
    outportb(REG_SCREEN_CTRL, 15);
    outportb(REG_SCREEN_DATA, temp & 0xFF);
}

// This function clears all the columns in a given row starting at 0xb80000
// which is the very first box on the screen left top corner
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

// This function calls Clear Row to clear the screen, It clears row by row
// and clearing the columns of each row one by one.  
void print_clear() {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
    row = 0;
    col = 0;
    updateCursor();
}

//
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

//
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

//
void print_str(char* string) {
   
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) string[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

// New function to print an integer
void print_int(uint16_t num) {
    // Convert the integer to a string for printing
    char num_str[12]; // Maximum of 11 digits for an integer (including sign)
    int_to_str(num, num_str, sizeof(num_str));

    // Print the stringr
    print_str(num_str);
}

// Function to convert an integer to a string
void int_to_str(uint16_t num, char* buffer, size_t buffer_size) {
    // Handle the case of 0 separately
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    // Initialize variables
    uint8_t i = 0;
    uint8_t is_negative = 0;

    // Handle negative numbers
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    // Build the string in reverse order
    while (num > 0 && i < buffer_size - 1) {
        buffer[i] = '0' + num % 10;
        num /= 10;
        i++;
    }

    // Add '-' for negative numbers
    if (is_negative && i < buffer_size - 1) {
        buffer[i] = '-';
        i++;
    }

    // Null-terminate the string
    buffer[i] = '\0';

    // Reverse the string
    uint8_t start = 0;
    uint8_t end = i - 1;
    while (start < end) {
        // Swap characters
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;

        // Move indices towards the center
        start++;
        end--;
    }
}

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

void print_tab()
{
    col += TAB_SIZE;
    updateCursor();
}

//
void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground | (background << 4);
}

void reset_key_buffer() {
    key_buffer = '\0';
    // Reset the buffer index
    key_buffer_index = 0;
}

//
uint16_t strLength(char* string) {

    uint16_t i = 1;

    while(string[i++]);

    return --i;
}

//
uint8_t strEqual(char* string1, char* string2) {
    uint8_t result = 1;
    uint8_t size = strLength(string1);

    if ( size != strLength(string2)) {
        // print_str("\nERR: sizes are not the same");
        result = 0;
    }

    else {
        uint8_t i = 0;

        for (i; i <= size; i++) {
            if(string1[i] != string2[i]) {
                // print_str("\nERR: strings do not match");
                result = 0;
            }
        }
    }
    return result;
}

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

void strncpy(char *dest, char *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        dest[i] = src[i];
    }
}

char* strChr(char* str, char ch) {
    while (*str != '\0') {
        if (*str == ch) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

// Every time you press a key, the keyboard send a signal to the PIC and triggers IRQ1 (Interrupt Request 1), 
// and the corresponding interrupt handler is called.
void keyboard_handler() {
    uint8_t status = inportb(KEYBOARD_STATUS_PORT);
    // Lowest bit of status will be set if buffer is not empty
    if (status & 0x1) {
        // Read the scancode from the keyboard hardware
        uint8_t scancode = inportb(KEYBOARD_DATA_PORT);
        // print scancodes into characters and handle special keys
        if (scancode == 0xE0) {
            scancode = inportb(KEYBOARD_DATA_PORT);
            switch (scancode) {
                case 0x48:  // Up arrow
                    if (row > 0) {
                        row--;
                        updateCursor();
                    }
                    break;
                case 0x50:  // Down arrow
                    if (row < NUM_ROWS - 1) {
                        row++;
                        updateCursor();
                    }
                    break;
                case 0x4B:  // Left arrow
                    if (col > 0) {
                        col--;
                        updateCursor();
                    }
                    break;
                case 0x4D:  // Right arrow
                    if (col < NUM_COLS - 1) {
                        col++;
                        updateCursor();
                    }
                    break;
            }
        }
        else {
            switch(scancode) {
                case 1:
                    print_char ((char)27);      // ESCAPE
                    break;
                case 2:
                    print_char ('1'); 
                    break; 
                case 3:
                    print_char ('2'); 
                    break;
                case 4:
                    print_char ('3'); 
                    break;
                case 5:
                    print_char ('4'); 
                    break;
                case 6:
                    print_char ('5'); 
                    break;
                case 7:
                    print_char ('6'); 
                    break;
                case 8:
                    print_char ('7'); 
                    break;
                case 9:
                    print_char ('8'); 
                    break;
                case 10:
                    print_char ('9'); 
                    break;
                case 11:
                    print_char ('0'); 
                    break;
                case 12:
                    print_char ('-'); 
                    break;
                case 13:
                    print_char ('='); 
                    break;
                case 14:
                    print_char('\b');   // BACKSPACE
                    break;
                case 15:
                    print_char('\t');   //TAB - IDENT
                    break;
                case 16:
                    print_char ('q'); 
                    break;
                case 17:
                    print_char ('w'); 
                    break;
                case 18:
                    print_char ('e'); 
                    break;
                case 19:
                    print_char ('r'); 
                    break;
                case 20:
                    print_char ('t'); 
                    break;
                case 21:
                    print_char ('y'); 
                    break;
                case 22:
                    print_char ('u'); 
                    break;
                case 23:
                    print_char ('i'); 
                    break;
                case 24:
                    print_char ('o'); 
                    break;
                case 25:
                    print_char ('p'); 
                    break;
                case 26:
                    print_char ('['); 
                    break;
                case 27:
                    print_char (']'); 
                    break;
                case 28:
                    print_char('\0');           //ENTER
                    if (strEqual(key_buffer, "clear")) {
                        print_clear();
                        print_set_color(GREEN, BLUE);
                    }
                    else if (strEqual(key_buffer, "create")) {

                    }

                    else {
                        print_str("\nERR: Bad command!");
                    }

                    print_str("\nJDOS> ");
                    reset_key_buffer();
                    break;
                // case 29:
                //     print_char (char)27;     //Left Control
                //     break;
                case 30:
                    print_char ('a'); 
                    break;
                case 31:
                    print_char ('s'); 
                    break;
                case 32:
                    print_char ('d'); 
                    break;
                case 33:
                    print_char ('f'); 
                    break;
                case 34:
                    print_char ('g'); 
                    break;
                case 35:
                    print_char ('h'); 
                    break;
                case 36:
                    print_char ('j'); 
                    break;
                case 37:
                    print_char ('k'); 
                    break;
                case 38:
                    print_char ('l'); 
                    break;
                case 39:
                    print_char (';'); 
                    break;
                case 40:
                    print_char ((char)39);
                    break;
                case 41:
                    print_char ((char)96);
                    break;
                // case 42:
                //     print_char (char)27);
                //     break;
                case 43:
                    print_char ((char)92);
                    break;
                case 44:
                    print_char ('z'); 
                    break;
                case 45:
                    print_char ('x'); 
                    break;
                case 46:
                    print_char ('c'); 
                    break;
                case 47:
                    print_char ('v'); 
                    break;
                case 48:
                    print_char ('b'); 
                    break;
                case 49:
                    print_char ('n'); 
                    break;
                case 50:
                    print_char ('m'); 
                    break;
                case 51:
                    print_char (','); 
                    break;
                case 52:
                    print_char ('.'); 
                    break;
                case 53:
                    print_char ('/'); 
                    break;
                case 57:
                    print_char (' ');           // SPACE BAR
                    break;
            }
        }   
        updateCursor();
    }
    // End of interupt
    outportb(PIC_MASTER_CMD, PIC_EOI);
}




