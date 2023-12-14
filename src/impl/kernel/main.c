#include "kb_print.h" 
#include "constants.h"
#include "ports.h"

enum {
    ExFAT,
    FAT12,
    FAT16,
    FAT32
 } fat_type;

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
    uint16_t table_size_16;
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
    char* file;
    BootSector boot_sector;
    uint32_t fat_offset;
    uint32_t data_offset;
    uint32_t current_cluster;
} FatFileSystem;

typedef uint8_t bool;
uint32_t one = 1;
uint32_t zero = 0;

BootSector boot_sector;
uint32_t FAT[MAX_CLUSTERS_FAT32];


uint32_t getNextCluster(uint32_t current_cluster);
void read_boot_sector(BootSector* bs);
void write_sector(uint32_t sector_number, char* buffer, uint32_t sector_size);
void write_cluster(uint32_t cluster, char* buffer, uint32_t buffer_size);


void read_sector(uint32_t sector_number, char* buffer, uint32_t sector_size)
{
    // Assuming that the disk is memory-mapped at address 0x100000
    uint8_t *disk = (uint8_t *)0x100000;

    // Calculate the address of the sector
    uint8_t *sector_address = disk + sector_number * sector_size;

    // Copy the sector into the buffer
   memCpy(buffer, sector_address, sector_size);
}

// Function to find a free cluster in the FAT
uint32_t find_free_cluster(uint32_t* FAT) {
    // Calculate the total number of sectors in the volume
    uint32_t total_sectors = boot_sector.total_sectors_32;
    uint32_t fat_size = boot_sector.sectors_per_fat_32;
    uint32_t root_dir_sectors = ((boot_sector.root_entry_count * 32) + (boot_sector.bytes_per_sector - 1)) / boot_sector.bytes_per_sector;
    uint32_t first_data_sector = boot_sector.reserved_sector_count + (boot_sector.fat_count * fat_size) + root_dir_sectors;
    uint32_t first_fat_sector = boot_sector.reserved_sector_count;
    uint32_t data_sectors = total_sectors - (boot_sector.reserved_sector_count + (boot_sector.fat_count * fat_size) + root_dir_sectors);
    uint32_t total_clusters = data_sectors / boot_sector.sectors_per_cluster;

    // Iterate through the clusters in the FAT
    for (uint32_t cluster = 2; cluster < total_clusters; cluster++) {
        if (FAT[cluster] == 0) {
            // Found a free cluster
            return cluster;
        }
    }

    // No free cluster found
    return 0;  // You might use a special value to indicate failure, like 0
}

// Function to find an empty directory entry in a cluster
uint32_t find_directory_entry(DirectoryEntry* entry, char* filename) {

    read_boot_sector(&boot_sector);
    // Calculate the sector number of the first sector in the cluster
    uint32_t root_dir_sectors = ((boot_sector.root_entry_count * 32) + (boot_sector.bytes_per_sector - 1)) / boot_sector.bytes_per_sector;
    uint32_t first_data_sector = boot_sector.reserved_sector_count + (boot_sector.fat_count * boot_sector.table_size_16) + root_dir_sectors;
    uint32_t first_sectorofRootDir = first_data_sector - root_dir_sectors;

    // Read the sector into a buffer
    char buffer[boot_sector.bytes_per_sector];
    read_sector(first_sectorofRootDir, buffer, boot_sector.bytes_per_sector);

    // Iterate over all directory entries in the cluster
    for (size_t i = 0; i < boot_sector.bytes_per_sector; i += 32) {
        // Check if the entry is unused
        if (strEqual((buffer + i), filename) == 0) {
            // Copy the directory entry into a structure
            memCpy(entry, buffer + i, sizeof(DirectoryEntry));
            return one;
        }
    }

    // No empty entry found in this cluster
    return zero;
}

// Function to allocate a new cluster in the FAT
uint32_t allocate_cluster() {

    read_boot_sector(&boot_sector);

    // Calculate the sector number of the first sector in the FAT
    uint32_t fat_sector = boot_sector.reserved_sector_count;
    
    // Calculate the size of the FAT in bytes
    uint32_t fat_size = boot_sector.sectors_per_fat_32 * boot_sector.bytes_per_sector;

    // Read the FAT into a buffer
    char fat_table[SECTOR_SIZE];
    read_sector(fat_sector, fat_table, fat_size);

    // Iterate over all entries in the FAT
    for (uint32_t i = 2; i < fat_size / FAT_ENTRY_SIZE; ++i) {
        if (*(uint32_t*)&fat_table[i * FAT_ENTRY_SIZE] == 0) {
            // Mark the cluster as in use
            *(uint32_t*)&fat_table[i * FAT_ENTRY_SIZE] = 0xFFFFFFFF;

            // Write the updated buffer back to the FAT
            write_sector(fat_sector, fat_table, fat_size);

            // Return the newly allocated cluster number
            return i;
        }
    }

    // No free clusters found
    return 0;
}

// Function to update FAT entries for a new cluster chain
void update_cluster(uint32_t start_cluster) {

    // Iterate through the cluster chain
    while (start_cluster < 0x0FFFFFF8) {
        // Calculate the offset in the FAT table
        uint32_t fat_offset = start_cluster * 4;

        // Calculate the sector number of the FAT table
        uint32_t fat_sector = boot_sector.reserved_sector_count + (fat_offset / boot_sector.bytes_per_sector);

        // Calculate the entry offset within the sector
        uint32_t ent_offset = fat_offset % boot_sector.bytes_per_sector;

        // Read the sector into a buffer
        char fat_table[boot_sector.bytes_per_sector];
        read_sector(fat_sector, fat_table, boot_sector.bytes_per_sector);

        // Update the FAT entry with the next cluster in the chain
        uint32_t next_cluster = getNextCluster(start_cluster);
        *(uint32_t*)&fat_table[ent_offset] = next_cluster;

        // Write the updated buffer back to the sector
        write_sector(fat_sector, fat_table, boot_sector.bytes_per_sector);
    }
}

uint32_t extractLittleEndian32(uint8_t* buffer, uint32_t offset) {
    return (buffer[offset] | (buffer[offset + 1] << 8) | (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24));
}

// Function to get the next cluster in the file chain
uint32_t getNextCluster(uint32_t current_cluster) {
    // Calculate the offset of the FAT entry for the current cluster
    uint32_t fat_offset = current_cluster * FAT_ENTRY_SIZE;

    // Calculate the sector and offset within the sector where the FAT entry is located
    uint32_t fat_sector = boot_sector.reserved_sector_count + (fat_offset / boot_sector.bytes_per_sector);
    uint32_t ent_offset = fat_offset % boot_sector.bytes_per_sector;

    // Read the sector containing the FAT entry
    // Implement the logic to read the sector at 'fatSector' into a buffer
    uint8_t buffer[boot_sector.bytes_per_sector];
    // Read the sector into the buffer (replace the following line with your actual read logic)
    read_sector(fat_sector, buffer, boot_sector.bytes_per_sector);

    // Extract the FAT entry from the buffer
    uint32_t fat_entry = extractLittleEndian32(buffer, ent_offset);

    // Check for end-of-file marker in FAT
    if (fat_entry >= 0x0FFFFFF8 && fat_entry <= 0x0FFFFFFF) {
        // End of file or bad cluster
        return 0;
    }

    // Return the next cluster
    return fat_entry;
}

void update_directory_entry(DirectoryEntry* entry, char* filename, char* extension, uint8_t attributes, uint32_t first_cluster, uint32_t file_size) {
    read_boot_sector(&boot_sector);

    // root directory sector
    uint32_t root_dir_sectors = ((boot_sector.root_entry_count * 32) + (boot_sector.bytes_per_sector - 1)) / boot_sector.bytes_per_sector;
    uint32_t first_data_sector = boot_sector.reserved_sector_count + (boot_sector.fat_count * boot_sector.table_size_16) + root_dir_sectors;
    uint32_t first_sectorofRootDir = first_data_sector - root_dir_sectors;

    // Read the root directory into a buffer
    char buffer[boot_sector.bytes_per_sector];
    read_sector(first_sectorofRootDir, buffer, boot_sector.bytes_per_sector);

    // Find an empty directory entry (first byte of filename will be 0x00)
    for (int i = 0; i < boot_sector.bytes_per_sector; i += 32) {
        if (buffer[i] == 0x00) {
            // Find an available cluster
            uint32_t firstCluster = allocate_cluster();

            // If no available cluster was found, print an error message and return
            if (firstCluster == 0) {
                // Replace this with your actual VGA print function
                print_str("\nNo available clusters found\n");
                return;
            }

            // Copy the filename and extension
            memCpy(buffer + i, filename, FILENAME_LENGTH);
            memCpy(buffer + i + 8, extension, EXTENSION_LENGTH);

            // Set the file attributes
            buffer[i + 11] = attributes;

            // Set the file size
            *(uint32_t*)(buffer + i + 28) = file_size;

            // Set the first cluster
            *(uint16_t*)(buffer + i + 26) = firstCluster;

            // Write the sector back to disk
            write_sector(first_sectorofRootDir, buffer, boot_sector.bytes_per_sector);

            // Replace this with your actual VGA print function
            print_str("\nFile created successfully: ");
            print_str(filename);
            print_str("\nFile size: ");
            print_int(file_size);
            return;
        }
    }
    
    // Copy the filename and extension
    memCpy(entry->filename, filename, FILENAME_LENGTH);
    memCpy(entry->ext, extension, EXTENSION_LENGTH);

    // Set the file attributes
    entry->attributes = attributes;

    // Set the file size
    entry->file_size = file_size;

    // Set the first cluster
    entry->cluster_low = first_cluster & 0xFFFF;
    entry->cluster_high = (first_cluster >> 16) & 0xFFFF;
}

void write_file(char* filename, uint32_t* FAT, uint32_t file_size) {
    // Find the directory entry for the file
    DirectoryEntry entry;
    if (!find_directory_entry(&entry, filename)) {
        // Replace this with your actual VGA print function
        print_str("\nFile not found\n");
        return;
    }

    // Allocate a new cluster chain for the file
    uint32_t first_cluster = allocate_cluster();
    update_cluster(first_cluster);
    FAT[first_cluster] = FAT32_EOF;

    // Update the directory entry
    update_directory_entry(&entry, filename, "txt", 0x20, first_cluster, file_size);

    // Write the file content to disk
    write_cluster(first_cluster, "Hello World!", file_size);

    // Replace this with your actual VGA print function
    print_str("\nFile written successfully\n");
}

void write_cluster(uint32_t cluster, char* buffer, uint32_t buffer_size)
{
    // Calculate the sector number of the first sector in the cluster
    uint32_t cluster_address = ((cluster - 2) * boot_sector.sectors_per_cluster) + boot_sector.reserved_sector_count + (boot_sector.fat_count * boot_sector.sectors_per_fat_32);

    // Read the sector into a buffer
    write_sector(cluster_address, buffer, buffer_size);
}

void write_sector(uint32_t sector_number, char * buffer, uint32_t sector_size)
{
    // Assuming that the disk is memory-mapped at address 0x100000
    uint8_t *disk = (uint8_t *)0x100000;

    // Calculate the address of the sector
    uint8_t *sector_address = disk + sector_number * sector_size;

    // Copy the sector into the buffer
   memCpy(sector_address, buffer, sector_size);
}

void read_boot_sector(BootSector* bs)
{
    // Disk is memory-mapped at address 0x100000
    uint8_t *disk = (uint8_t *)0x100000;

    // Copy the boot sector into the structure
    memCpy(bs, disk, sizeof(BootSector));

}

void create_file(char *filename, FatFileSystem* fs, uint32_t* FAT) {
    // Find an available cluster
    uint32_t first_cluster = find_free_cluster(FAT);

    DirectoryEntry newEntry;
    newEntry.filename[0] = 0x00;
    newEntry.attributes = 0x20;
    newEntry.cluster_low = 0x0000;
    newEntry.cluster_high = 0x0000;
    newEntry.file_size = 0x00000000;
    newEntry.ext[0] = 0x00;
    newEntry.reserved = ' ';
    newEntry.create_date = 0x0000;
    newEntry.create_time = 0x0000;
    newEntry.last_access_date = 0x0000;
    newEntry.last_mod_date = 0x0000;
    newEntry.last_mod_time = 0x0000;

    // Update the FAT entries to link the clusters
    update_cluster(first_cluster);
    FAT[first_cluster] = FAT32_EOF;

    uint32_t first_sectorOfRootDir = ((first_cluster - 2) * fs->boot_sector.sectors_per_cluster) + fs->boot_sector.reserved_sector_count + (fs->boot_sector.fat_count * fs->boot_sector.table_size_16) + ((fs->boot_sector.root_entry_count * 32) + (fs->boot_sector.bytes_per_sector - 1)) / fs->boot_sector.bytes_per_sector;
    uint8_t buffer[fs->boot_sector.bytes_per_sector];
    read_sector(first_sectorOfRootDir, buffer, fs->boot_sector.bytes_per_sector);

    // Find an empty directory entry (first byte of filename will be 0x00)
    for (int i = 0; i < fs->boot_sector.bytes_per_sector; i += 32) {
        if (buffer[i] == 0x00) {
            // Copy the filename and extension
            memCpy(buffer + i, filename, FILENAME_LENGTH);
            memCpy(buffer + i + 8, "txt", EXTENSION_LENGTH);

            // Set the file attributes
            buffer[i + 11] = 0x20;

            // Set the file size
            *(uint32_t*)(buffer + i + 28) = 0;

            // Set the first cluster
            *(uint16_t*)(buffer + i + 26) = first_cluster;

            // Write the sector back to disk
            write_sector(first_sectorOfRootDir, buffer, fs->boot_sector.bytes_per_sector);

            // Replace this with your actual VGA print function
            print_str("\nFile created successfully: ");
            print_str(filename);
            print_str("\nFile size: ");
            print_int(0);
            return;
        }
        else {
            print_str("No empty directory entry found\n");
        }
    }

    // Update the directory entry
    update_directory_entry(&newEntry, filename, "txt", 0x20, first_cluster, 0);

    // Write the file content to disk
    write_file(filename, FAT, 0);
}

// This function reads the boot sector, detects the FAT type, and initializes the file system structure.
void initialize_fat_file_system(FatFileSystem* fs, char* file)
{
    // Read the boot sector
    read_boot_sector(&fs->boot_sector);
    // Default root cluster for FAT32
    fs->boot_sector.root_cluster = 2;

    // Calculate offset of the FAT table
    uint32_t fat_offset = fs->boot_sector.reserved_sector_count;
    // Calculate offset of the data region
    uint32_t data_offset = fat_offset + fs->boot_sector.fat_count * fs->boot_sector.sectors_per_fat_32;
    // Calculate offset of the root directory
    uint32_t root_offset = data_offset + fs->boot_sector.root_cluster * fs->boot_sector.sectors_per_cluster;

    // Initialize the file system structure
    fs->file = file;
    fs->fat_offset = fat_offset;
    fs->data_offset = data_offset;
    fs->current_cluster = fs->boot_sector.root_cluster;

     // Total sectors in volume (including VBR):
    uint32_t total_sectors = (fs->boot_sector.total_sectors_32 == 0) ? fs->boot_sector.total_sectors_32 : fs->boot_sector.total_sectors_16;
    // FAT size in sectors:
    uint32_t fat_size = fs->boot_sector.sectors_per_fat_32 * fs->boot_sector.bytes_per_sector;
    // Sectors occupied by the root directory:
    uint32_t root_dir_sectors = ((fs->boot_sector.root_entry_count * 32) + (fs->boot_sector.bytes_per_sector - 1)) / fs->boot_sector.bytes_per_sector;
    // First data sector number:
    uint32_t first_data_sector = fs->boot_sector.reserved_sector_count + (fs->boot_sector.fat_count * fat_size) + root_dir_sectors;
    // Total data sectors:
    uint32_t first_fat_sector = fs->boot_sector.reserved_sector_count;
    // Total data sectors:
    uint32_t data_sectors = total_sectors - (fs->boot_sector.reserved_sector_count + (fs->boot_sector.fat_count * fat_size) + root_dir_sectors);
    // Total clusters in volume:
    uint32_t total_clusters = data_sectors / fs->boot_sector.sectors_per_cluster;

    // Calculate the sector number of the first sector in the cluster
    if (SECTOR_SIZE == 0) {
        // This is an ExFAT file system
        print_str("Detected ExFAT\n");
        fat_type = ExFAT;
    }
    else if (total_clusters < 4085)
    {
        // FAT12
        print_str("Detected FAT12\n");
        fat_type = FAT12;
    }
    else if (total_clusters < 65525)
    {
        // FAT16
        print_str("Detected FAT16\n");
        fat_type = FAT16;
    }
    else
    {
        // FAT32
        print_str("Detected FAT32\n");
        fat_type = FAT32;
    }

    // Output extracted information
    print_str("Total Sectors: ");
    print_int (total_sectors);
    print_str("\n");
    print_str("Sectors per Cluster: ");
    print_int (fs->boot_sector.sectors_per_cluster);
    print_str("\n");
    print_str("FAT Size (in sectors): ");
    print_int (fat_size);
    print_str("\n");
    print_str("Root Directory Sectors: ");
    print_int (root_dir_sectors);
    print_str("\n");
    print_str("First Data Sector: ");
    print_int (first_data_sector);
    print_str("\n");
    print_str("First FAT Sector: ");
    print_int (first_fat_sector);
    print_str("\n");
    print_str("Data Sectors: ");
    print_int (data_sectors);
    print_str("\n");
    print_str("Total Clusters: ");
    print_int (total_clusters);
    print_str("\n");

    // This allows you to zero-index clusters:
    print_str("Cluster size: ");
    uint64_t clustersize = fs->boot_sector.sectors_per_cluster * SECTOR_SIZE;
    print_int (clustersize);
    print_str("\n");
    print_str("Cluster Heap Offset: ");
    uint64_t clusterheapoffset = fat_offset + fs->boot_sector.fat_count * fs->boot_sector.sectors_per_fat_32;
    print_int (clusterheapoffset);
    print_str("\n");
    print_str("Cluster Array: ");
    uint64_t clusterArray = clusterheapoffset * SECTOR_SIZE - 2 * clustersize;
    print_int (clusterArray);
    print_str("\n");
    print_str("Cluster Count: ");
    uint64_t clustercount = fs->boot_sector.total_sectors_32 / fs->boot_sector.sectors_per_cluster;
    print_int (clustercount);
    print_str("\n");
    print_str("Usable Space: ");
    uint64_t usablespace = clustercount * clustersize;
    print_int (usablespace);
    print_str("\n");
    print_str("Fat Offset: ");
    uint64_t fatoffset = fat_offset * SECTOR_SIZE;
    print_int (fatoffset);
    print_str("\n");
}



extern void load_IDT(void);

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
    print_str("Welcome to JDOS operating system\n");
    print_newline();

    FatFileSystem fs;
    initialize_fat_file_system(&fs, "hdd.img");
    print_str("File system initialized\n");
    print_newline();
    print_str("Creating file...\n");
    print_newline();

    print_str("\nJDOS> ");

    idt_init();

    while (1);
}