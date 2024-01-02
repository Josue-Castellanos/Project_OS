#include "constants.h"
#include "ports.h"
#include "vga.h"
#include "keyboard.h"
#include "str_manip.h"
#include "common.h"


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
                        print_set_color(GREEN, BLACK);
                    }
                    else if (strEqual(key_buffer, "create")) {

                    }

                    else {
                        print_set_color(BRIGHT_GREEN, BLACK);
                        print_str("\nERR: Bad command!");
                    }
                    print_set_color(MAGENTA, BLACK);
                    print_str("\nJDOS> ");
                    print_set_color(GREEN, BLACK);
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