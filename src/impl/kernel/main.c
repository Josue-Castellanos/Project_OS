#include "vga.h" 
#include "constants.h"
#include "ports.h"
#include "fat_32.h"
#include "hdd.h"
#include "idt.h"



void kernel_main() {
    print_clear();
    print_set_color(MAGENTA, BLACK);
    print_str("Welcome to JDOS operating system\n");
    print_newline();

    char buffer[SECTOR_SIZE];
    FatFileSystem fs;
    initialize_fat_file_system(&fs, "hdd.img");
    print_newline();

    char* filename = "test.txt";
    create_file("test.txt", &fs);

    print_set_color(MAGENTA, BLACK);
    print_str("\nJDOS> ");
    print_set_color(GREEN, BLACK);
    reset_key_buffer();
  
    idt_init();


    while (1);
}