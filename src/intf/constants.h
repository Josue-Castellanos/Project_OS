#ifndef CONSTANTS_H
#define CONTSTANTS_H

#define IDT_SIZE 256
#define MAX_SIZE 30

#define KEYBOARD_DATA_PORT 0x60     /* IO Ports for Keyboard */
#define KEYBOARD_STATUS_PORT 0x64

#define MASK_ALL_INTS 0xFF      /* Mask all interupts in PIC */

#define PIC_MASTER_CMD 0x20     /* IO base address for master PIC */
#define PIC_MASTER_DATA 0x21    

#define PIC_SLAVE_CMD 0xA0      /* IO base address for slave PIC */
#define PIC_SLAVE_DATA 0xA1

#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

#define INTERRUPT_GATE 0x8E     /* 64-bit Interrupt Gate: 0x8E 
                                (p=1, dpl=0b00, type=0b1110 => 
                                type_attributes=0b1000_1110=0x8E) */
#define KERNEL_CODE_SEGMENT_OFFSET 0x08  /* CS address */

#define VGA_MAX_WIDTH 80
#define VGA_MAX_HEIGHT 25

#define ICW1_ICW4_INIT 0x11		/* Indicates that ICW4 will be present */
                                /* Initialization - ired! */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define PIC_EOI		0x20		/* End-of-interrupt command code */
#define KEYBOARD_IRQ 1          /* Standard ISA Interupt est 1 - Keyboard Interupt*/

#define FAT_EOF FAT32_EOF       /* Define a generic EOF constant */
#define FAT32_EOF 0x0FFFFFFF    /* FAT32 EOF constant */

#define SECTOR_SIZE 512

#define FAT_ENTRY_SIZE 4

#define CLUSTER_SIZE 4096

#define FILE_SIZE 1024

#define FAT32_CLUSTER_SIZE 4096 /* FAT32 cluster size in bytes */

#define FAT32_ROOT_DIR_CLUSTER 2    /* FAT32 root directory cluster */

#define FAT32_MAX_FILE_NAME_LENGTH 255 /* FAT32 max file name length */

#define FAT32_MAX_PATH_LENGTH 4096 /* FAT32 max path length */

#define FILENAME_LENGTH 8

#define EXTENSION_LENGTH 3

#define END_OF_FILE_MARKER 0x0FFFFFFF

#define BAD_CLUSTER_MARKER 0xFFF7

#define MAX_CLUSTERS_FAT32 0x0FFFFFF7  // FAT32 supports up to 0xFFFFFF7 clusters (0xFFFFFF8 to 0xFFFFFFFF are reserved)

#define FAT32_MAX_FILES_PER_DIR 512 /* FAT32 max files per directory */

#define FAT32_MAX_DIRS_PER_CLUSTER 65536 /* FAT32 max directories per cluster */

#define O_RDONLY    0

#define O_WRONLY    1

#define O_RDWR      2

#define O_CREAT     64

#define O_APPEND    1024

#define O_DIRECT    16384

#define O_DIRECTORY 65536

#define O_NOFOLLOW  131072

#define O_NOATIME   262144

#define O_CLOEXEC   524288

#define O_SYNC      1052672

#define O_PATH      2097152

#define O_TMPFILE   4259840 

#define O_TRUNC     0x0400

#define EXIT_FAILURE 1

#endif