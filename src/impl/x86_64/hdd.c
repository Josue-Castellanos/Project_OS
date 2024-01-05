#include "fat_32.h"
#include "hdd.h"
#include "constants.h"
#include "memory.h"
#include "vga.h"

void read_sector(uint32_t sector_number, char* buffer, uint32_t sector_size)
{
    // Assuming that the disk is memory-mapped at address 0x100000
    uint8_t *disk = (uint8_t *)0x100000;

    // Calculate the address of the sector
    uint8_t *sector_address = disk + sector_number * sector_size;

    // Copy the sector into the buffer
   memCpy(buffer, sector_address, sector_size);
}

// This function reads the next cluster in the chain.
uint32_t read_fat_table(uint32_t active_cluster, uint32_t first_fat_sector)
{
    char FAT_table[boot_sector.bytes_per_sector];
    uint32_t fat_offset = active_cluster * 4;
    uint32_t fat_sector = first_fat_sector + (fat_offset / SECTOR_SIZE);
    uint32_t ent_offset = fat_offset % SECTOR_SIZE;

    //at this point you need to read from sector "fat_sector" on the disk into "FAT_table".
    read_sector(fat_sector, FAT_table, boot_sector.bytes_per_sector);

    //remember to ignore the high 4 bits.
    uint32_t table_value = *(uint32_t*)&FAT_table[ent_offset];
    if (fatType == FAT32) table_value &= 0x0FFFFFFF;

    // The variable "table_value" now has the information you need about the next cluster in the chain.
    if (table_value >= 0x0FFFFFF8)
    {
        // There are no more clusters in the chain. This means that the whole file has been read.
        print_str("End of file reached.\n");
    }
    else if (table_value == 0x0FFFFFF7)
    {
        // This cluster has been marked as "bad". "Bad" clusters are prone to errors and should be avoided.
        print_str("Bad cluster encountered.\n");
    }
    else
    {
        // "table_value" is the cluster number of the next cluster in the file.
        print_str("Next cluster in the file: ");
        print_char(table_value);
        print_str("\n");
    }
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

void write_cluster(uint32_t cluster, char* buffer, uint32_t buffer_size)
{
    // Calculate the sector number of the first sector in the cluster
    uint32_t cluster_address = ((cluster - 2) * boot_sector.sectors_per_cluster) + boot_sector.reserved_sector_count + (boot_sector.fat_count * boot_sector.sectors_per_fat_32);

    // Read the sector into a buffer
    write_sector(cluster_address, buffer, buffer_size);
}

void write_sector(uint32_t sector_number, char* buffer, uint32_t sector_size)
{
    // Assuming that the disk is memory-mapped at address 0x100000
    uint8_t *disk = (uint8_t *)0x100000;

    // Calculate the address of the sector
    uint8_t *sector_address = disk + sector_number * sector_size;

    // Copy the sector into the buffer
   memCpy(sector_address, buffer, sector_size);
}

// Function to allocate a cluster in the FAT and return its cluster number
uint32_t allocate_cluster() {
    uint32_t cluster;
    uint32_t fat_entry;

    // Iterate through each FAT entry to find a free cluster
    for (cluster = 2; cluster < boot_sector.total_clusters + 2; cluster++) {
        fat_entry = get_fat_entry(cluster);

        print_set_color(RED, BLACK);
        print_str("\nfat_entry: ");
        print_int(fat_entry);
        print_str("\n");
        // Check if the cluster is free (0x00000000 in FAT32)
        if (fat_entry == 0x00000000) {
            // Mark the cluster as occupied in the FAT
            set_fat_entry(cluster, 0xFFFFFFFF);

            // Clear the cluster's data in the data region (optional step)
            clear_cluster_data(cluster);

            return cluster;
        }
    }

    // No free clusters found
    return 0;
}

// Helper function to get the value of a FAT entry given the cluster number
uint32_t get_fat_entry(uint32_t cluster) {
    // Calculate the offset in bytes for the FAT entry corresponding to the cluster
    uint32_t fat_offset = cluster * 4;

    // Read the FAT entry from the FAT table
    uint32_t fat_entry;
    read_fat_entry(fat_offset, &fat_entry);

    return fat_entry;
}

// Helper function to set the value of a FAT entry given the cluster number
void set_fat_entry(uint32_t cluster, uint32_t value) {
    // Calculate the offset in bytes for the FAT entry corresponding to the cluster
    uint32_t fat_offset = cluster * 4;

    // Write the new value to the FAT entry in the FAT table
    write_fat_entry(fat_offset, value);
}

// Helper function to read a FAT entry from the FAT table
void read_fat_entry(uint32_t offset, uint32_t* value) {
    // Calculate the sector and offset within the sector for the FAT entry
    uint32_t fat_sector = boot_sector.reserved_sector_count + (offset / boot_sector.bytes_per_sector);
    uint32_t fat_offset = offset % boot_sector.bytes_per_sector;

    // Read the sector containing the FAT entry
    char fat_buffer[boot_sector.bytes_per_sector];
    read_sector(fat_sector, fat_buffer, boot_sector.bytes_per_sector);

    // Copy the FAT entry value from the buffer
    memCpy(value, fat_buffer + fat_offset, sizeof(uint32_t));
}

// Helper function to write a FAT entry to the FAT table
void write_fat_entry(uint32_t offset, uint32_t value) {
    // Calculate the sector and offset within the sector for the FAT entry
    uint32_t fat_sector = boot_sector.reserved_sector_count + (offset / boot_sector.bytes_per_sector);
    uint32_t fat_offset = offset % boot_sector.bytes_per_sector;

    // Read the sector containing the FAT entry
    char fat_buffer[boot_sector.bytes_per_sector];
    read_sector(fat_sector, fat_buffer, boot_sector.bytes_per_sector);

    // Copy the new FAT entry value to the buffer
    memCpy(fat_buffer + fat_offset, &value, sizeof(uint32_t));

    // Write the updated sector back to disk
    write_sector(fat_sector, fat_buffer, boot_sector.bytes_per_sector);
}

// Helper function to clear the data in a cluster (optional step)
void clear_cluster_data(uint32_t cluster) {
    // Calculate the sector number where the cluster's data begins
    uint32_t data_sector = boot_sector.reserved_sector_count +
        (boot_sector.fat_count * boot_sector.sectors_per_fat_32) +
        ((cluster - 2) * boot_sector.sectors_per_cluster);

    // Clear the data in the cluster (set to 0x00)
    char zero_buffer[boot_sector.bytes_per_sector];
    memSet(zero_buffer, 0x00, boot_sector.bytes_per_sector);

    // Write the zeroed data to each sector in the cluster
    for (uint32_t i = 0; i < boot_sector.sectors_per_cluster; i++) {
        write_sector(data_sector + i, zero_buffer, boot_sector.bytes_per_sector);
    }
}


// // Function to allocate a new cluster in the FAT
// uint32_t allocate_cluster(BootSector* bs) {
//     read_boot_sector(bs);

//     // Calculate the sector number of the first sector in the FAT
//     uint32_t fat_sector = bs->reserved_sector_count;

//     // Calculate the size of the FAT in bytes
//     uint32_t fat_size = bs->sectors_per_fat_32 * bs->bytes_per_sector;

//     // Read the FAT into a buffer
//     char fat_table[SECTOR_SIZE];
//     read_sector(fat_sector, fat_table, fat_size);

//     // Iterate over all entries in the FAT
//     for (uint32_t i = 2; i < fat_size / FAT_ENTRY_SIZE; ++i) {
//         if (*(uint32_t*)&fat_table[i * FAT_ENTRY_SIZE] == 0) {
//             // Mark the cluster as in use
//             *(uint32_t*)&fat_table[i * FAT_ENTRY_SIZE] = 0xFFFFFFFF;

//             // Write the updated buffer back to the FAT
//             write_sector(fat_sector, fat_table, fat_size);

//             // Return the newly allocated cluster number (add 2 since clusters start from 2)
//             return i + 2;
//         }
//     }

//     // No free clusters found
//     return 0;
// }

