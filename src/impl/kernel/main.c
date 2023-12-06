#include "kb_print.h" 
#include "constants.h"
#include "ports.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void load_IDT(void);


#define SECTOR_SIZE 512
#define FAT_ENTRY_SIZE 4
#define CLUSTER_SIZE 4096

typedef uint8_t bool;
#define true 1;
#define false 0;

typedef struct {
    // BPB (BIOS Parameter Block)
    uint8_t jump[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_descriptor_type;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors_count;
    uint32_t total_sectors_32;

    // Extended boot record
    uint32_t sectors_per_fat_32;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_cluster_count;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t signature;
    uint32_t volume_id;
    char volume_label[11];  // 11 spaces
    char file_system_type[8];   // Always FAT32

    // FSInfo Structure 
    uint32_t fs_info_lead_signature;
    uint8_t fs_info_reserved1[480];
    uint32_t fs_info_struct_signature;
    uint32_t fs_info_free_cluster_count;
    uint32_t fs_info_next_free_cluster_hint;
    uint8_t fs_info_reserved2[12];
    uint32_t fs_info_trail_signature;

    // Additional field for root cluster
    uint32_t root_cluster;
} __attribute__((packed)) BootSector;

typedef struct {
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t cluster_high;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t cluster_low;
    uint32_t file_size;
} __attribute__((packed)) DirectoryEntry;

typedef struct {
    FILE* file;
    BootSector boot_sector;
    uint32_t fat_offset;
    uint32_t data_offset;
    uint32_t current_cluster;
} FatFileSystem;

uint32_t gRootDirectoryEnd;
uint8_t* gFat = NULL;
DirectoryEntry* entry = NULL;
BootSector boot_sector;

bool read_boot_sector0(FatFileSystem* fs) {
    fseek(fs->file, 0, SEEK_SET);
    return fread(&boot_sector, sizeof(BootSector), 1, fs->file) > 0;
}

bool read_sectors(FatFileSystem* fs, uint32_t lba, uint32_t count, void* bufferOut)
{
    bool ok = true;
    ok = ok && (fseek(fs, lba * boot_sector.bytes_per_sector, SEEK_SET) == 0);
    ok = ok && (fread(bufferOut, boot_sector.bytes_per_sector, count, fs) == count);
    return ok;
}

bool read_Fat(FatFileSystem* fs)
{
    gFat = (uint8_t*) malloc(boot_sector.sectors_per_fat_32 * boot_sector.bytes_per_sector);
    return readSectors(fs, boot_sector.reserved_sector_count, boot_sector.sectors_per_fat_32, gFat);
}

DirectoryEntry* findFile(const char* name)
{
    for (uint32_t i = 0; i < boot_sector.root_entry_count; i++)
    {
        if (memcmp(name, entry[i].filename, 11) == 0)
            return &entry[i];
    }

    return NULL;
}

bool readFile(DirectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer)
{
    bool ok = true;
    uint16_t currentCluster = fileEntry->cluster_low;

    do {
        uint32_t lba = gRootDirectoryEnd + (currentCluster - 2) * boot_sector.sectors_per_cluster;
        ok = ok && readSectors(disk, lba, boot_sector.sectors_per_cluster, outputBuffer);
        outputBuffer += boot_sector.sectors_per_cluster * boot_sector.bytes_per_sector;

        uint32_t fatIndex = currentCluster * 3 / 2;
        if (currentCluster % 2 == 0)
            currentCluster = (*(uint16_t*)(gFat + fatIndex)) & 0x0FFF;
        else
            currentCluster = (*(uint16_t*)(gFat + fatIndex)) >> 4;

    } while (ok && currentCluster < 0x0FF8);

    return ok;
}

void read_boot_sector(FatFileSystem* fs) {
    fseek(fs->file, 0, SEEK_SET);
    fread(&boot_sector, sizeof(BootSector), 1, fs->file);
}

void read_fs_info(FatFileSystem* fs) {
    fseek(fs->file, SECTOR_SIZE, SEEK_SET); // FS Info is usually in sector 1
    fread(&boot_sector.fs_info_lead_signature, sizeof(BootSector) - offsetof(BootSector, fs_info_lead_signature), 1, fs->file);
}

void initialize_fat_file_system(FatFileSystem* fs, const char* filename) {
    fs->file = fopen(filename, "rb");
    if (!fs->file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if (!read_boot_sector0(fs->file)) {
        perror("Could not read boot sector!\n");
        exit(EXIT_FAILURE);
    }

    if (!readFat(fs)) {
        perror("Could not read FAT!\n");
        free(gFat);
        exit(EXIT_FAILURE);
    }

    if (!readRootDirectory(fs)) {
        fprintf(stderr, "Could not read FAT!\n");
        free(gFat);
        free(entry);
        exit(EXIT_FAILURE);
    }

    // DirectoryEntry* fileEntry = findFile(name);
    // if (!fileEntry) {
    //     fprintf(stderr, "Could not find file %s!\n", name);
    //     free(gFat);
    //     free(entry);
    //     exit(EXIT_FAILURE);
    // }

    // uint8_t* buffer = (uint8_t*) malloc(entry->file_size + boot_sector.bytes_per_sector);
    // if (!readFile(entry, fs, buffer)) {
    //     fprintf(stderr, "Could not read file %s!\n", name);
    //     free(gFat);
    //     free(entry);
    //     free(buffer);
    //     exit(EXIT_FAILURE);
    // }

    read_boot_sector(fs);
    read_fs_info(fs);

    fs->fat_offset = boot_sector.reserved_sector_count * SECTOR_SIZE;
    fs->data_offset = fs->fat_offset + boot_sector.fat_count * boot_sector.sectors_per_fat_32 * SECTOR_SIZE;
    fs->current_cluster = boot_sector.root_cluster;
}

void read_fat_entry(FatFileSystem* fs, uint32_t cluster, uint32_t* next_cluster) {
    fseek(fs->file, fs->fat_offset + cluster * FAT_ENTRY_SIZE, SEEK_SET);
    fread(next_cluster, FAT_ENTRY_SIZE, 1, fs->file);
}

void read_cluster(FatFileSystem* fs, uint32_t cluster, void* buffer) {
    fseek(fs->file, fs->data_offset + (cluster - 2) * boot_sector.sectors_per_cluster * SECTOR_SIZE, SEEK_SET);
    fread(buffer, boot_sector.sectors_per_cluster * SECTOR_SIZE, 1, fs->file);
}

void print_directory_entries(FatFileSystem* fs, uint32_t cluster) {
    DirectoryEntry entry;
    read_cluster(fs, cluster, &entry);

    while (entry.filename[0] != 0) {
        printf("Filename: %.8s\n", entry.filename);
        printf("Extension: %.3s\n", entry.ext);
        printf("Size: %u bytes\n", entry.file_size);
        printf("\n");

        read_cluster(fs, cluster, &entry);
    }
}

void close_fat_file_system(FatFileSystem* fs) {
    fclose(fs->file);
}


// ********************************************************************************************************************* 



// Define an interrupt descriptor structure
struct IDT_entry {
  uint16_t offset_low;     // Lower 16 bits of the interrupt handler function
  uint16_t selector;       // Code segment selector
  uint8_t ist;             // Interrupt Stack Table (only used on x86_64)
  uint8_t type_attr;       // Type and attributes
  uint16_t offset_mid;     // Middle 16 bits of the interrupt handler function
  uint32_t offset_high;    // Upper 32 bits of the interrupt handler function
  uint32_t zero;       // Reserved, set to zero
};

void remapPIC(void) {
	uint8_t a1, a2;
 
	a1 = inportb(PIC_MASTER_DATA);       // save masks
	a2 = inportb(PIC_SLAVE_DATA);
 
    // ICW1 - Initialize
    outportb(PIC_MASTER_CMD, ICW1_ICW4_INIT);  // Start initialization sequence (ICW1)
    outportb(PIC_SLAVE_CMD, ICW1_ICW4_INIT);

    // ICW2 - Set the base interrupt vectors
    outportb(PIC_MASTER_DATA, 0x20);  // Offset for master PIC (IRQs 0-7)
                                      // Place holder starting at 32
    outportb(PIC_SLAVE_DATA, 0x28);   // Offset for slave PIC (IRQs 8-15)
                                      // Place holder starting at 40
    // ICW3 - Configure cascading (master to slave and slave to master)
    outportb(PIC_MASTER_DATA, ICW1_INTERVAL4);  // Tell master PIC that there is a slave at IRQ2
    outportb(PIC_SLAVE_DATA, ICW1_SINGLE);   // Tell slave PIC its cascade identity

    // ICW4 - Additional configuration
    outportb(PIC_MASTER_DATA, ICW4_8086);  // Set master PIC to 8086/88 (MCS-80/85) mode
    outportb(PIC_SLAVE_DATA, ICW4_8086);   // Set slave PIC to 8086/88 (MCS-80/85) mode

    outportb(PIC_MASTER_DATA, a1);   // restore saved masks.
	outportb(PIC_SLAVE_DATA, a2);

    // Masking all bits in bot PICs
    outportb(PIC_MASTER_DATA, MASK_ALL_INTS);  // Mask all interrupts 
    outportb(PIC_SLAVE_DATA, MASK_ALL_INTS);   // Mask all interrupts 
}

void idt_init(void) {
  extern struct IDT_entry IDT[IDT_SIZE];
  extern void irq1();
  // ISR - The addresses of individual Interrupt Service Routines (ISRs) for
  // hardware interrupts (IRQs) are declared. These addresses correspond to functions
  // like irq0, irq1, and so on, which are responsible for handling specific interrupts.

  // Fill the IDT descriptor
  uint64_t irq_address = (uint64_t)irq1;
    uint8_t IRQ1 = 33; 
    IDT[IRQ1].offset_low = (uint16_t)(irq_address & 0xFFFF);
    IDT[IRQ1].selector = KERNEL_CODE_SEGMENT_OFFSET; // KERNEL_CODE_SEGMENT_OFFSET
    IDT[IRQ1].ist = 0;
    IDT[IRQ1].type_attr = INTERRUPT_GATE; // 64-bit INTERRUPT_GATE
    IDT[IRQ1].offset_mid = (uint16_t)((irq_address & 0xFFFF0000) >> 16);
    IDT[IRQ1].offset_high = (uint32_t)((irq_address & 0xFFFFFFFF00000000) >> 32);
    IDT[IRQ1].zero = 0;

    // Initiating PIC
    remapPIC();

    // Unmask IRQ1 -> 0xFD = 1111 1101 in binary enables only IRQ1
    outportb(PIC_MASTER_DATA, 0xFD);  // Mask all other interupts

    load_IDT();
}

void set_mask_IRQ() {
    uint16_t port;
    unsigned char IRQline = KEYBOARD_IRQ;
    uint8_t value;
 
    if(IRQline < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        IRQline -= 8;
    }
    value = inportb(port) | (1 << IRQline);
    outportb(port, value);        
}

void kernel_main() {
    print_clear();
    print_set_color(GREEN, BLUE);
//     char jdos_image[35] = { // 80 width 40 height
// "                         _....JDOS...._                       ",
// "                  _.gd$$$$$$$$$$$$$$$$$$bp._                  ""
// "               .g$$$$$$P^^""j$$b""""^^T$$$$$$p.               ""
// "            .g$$$P^T$$b    d$P T;       ""^^T$$$p.            ""
// "          .d$$P^"  :$; `  :$;                "^T$$b.          ""
// "        .d$$P'      T$b.   T$b                  `T$$b.        "
// "        d$$P'      .gg$$$$bpd$$$p.d$bpp.           `T$$b       ""
// "      d$$P      .d$$$$$$$$$$$$$$$$$$$$bp.           T$$b      ""
// ""     d$$P      d$$$$$$$$$$$$$$$$$$$$$$$$$b.          T$$b     ""
// ""    d$$P      d$$$$$$$$$$$$$$$$$$P^^T$$$$P            T$$b    ""
// ""   d$$P    '-'T$$$$$$$$$$$$$$$$$$bggpd$$$$b.           T$$b   ""
// ""  :$$$      .d$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$p._.g.     $$$;  ""
// ""  $$$;     d$$$$$$$$$$$$$$$$$$$$$$$P^"^T$$$$P^^T$$$;    :$$$  "
// "  :$$$     :$$$$$$$$$$$$$$:$$$$$$$$$_    "^T$bpd$$$$,     $$$; ""
// "  $$$;     :$$$$$$$$$$$$$$bT$$$$$P^^T$p.    `T$$$$$$;     :$$$ "
// " :$$$      :$$$$$$$$$$$$$$P `^^^'    "^T$p.    lb`TP       $$$;"
// ":$$$      $$$$$$$$$$$$$$$              `T$$p._;$b         $$$;
// "$$$;      $$$$$$$$$$$$$$;                `T$$$$:Tb        :$$$
// "$$$;      $$$$$$$$$$$$$$$                        Tb    _  :$$$
// ":$$$     d$$$$$$$$$$$$$$$.                        $b.__Tb $$$;
// ":$$$  .g$$$$$$$$$$$$$$$$$$$p...______...gp._      :$`^^^' $$$;
// "" $$$;  `^^'T$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$p.    Tb._, :$$$ 
// " :$$$       T$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$b.   "^"  $$$; 
// "  $$$;       `$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$b      :$$$  
// "  :$$$        $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$;     $$$;  
// "   T$$b    _  :$$`$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$;   d$$P   
// "    T$$b   T$g$$; :$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$  d$$P    
// "     T$$b   `^^'  :$$ "^T$$$$$$$$$$$$$$$$$$$$$$$$$$$ d$$P     
// "      T$$b        $P     T$$$$$$$$$$$$$$$$$$$$$$$$$;d$$P      
// ""       T$$b.      '       $$$$$$$$$$$$$$$$$$$$$$$$$$$$P       
// "        `T$$$p.   bug    d$$$$$$$$$$$$$$$$$$$$$$$$$$P'        
// ""          `T$$$$p..__..g$$$$$$$$$$$$$$$$$$$$$$$$$$P'          
// "            "^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$^"            
// "               "^T$$$$$$$$$$$$$$$$$$$$$$$$$$P^"               
// "                   """^^^T$$$$$$$$$$P^^^"""
//     };
    print_str("Welcome to JDOS operating system\nPlease enter a command\n");
    print_newline();
    print_str("\nJDOS> ");
    idt_init();

    FatFileSystem fs;
    initialize_fat_file_system(&fs, "hdd.img");

    // Access FS Info structure fields
    printf("Free Cluster Count: %u\n", fs.boot_sector.fs_info_free_cluster_count);
    printf("Next Free Cluster Hint: %u\n", fs.boot_sector.fs_info_next_free_cluster_hint);

    print_directory_entries(&fs, fs.boot_sector.root_cluster);

    close_fat_file_system(&fs);

    while (1);
}