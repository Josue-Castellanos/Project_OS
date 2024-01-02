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

void free_clusters(uint32_t start_cluster) {
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
        *(uint32_t*)&fat_table[ent_offset] = 0;

        // Write the updated buffer back to the sector
        write_sector(fat_sector, fat_table, boot_sector.bytes_per_sector);
    }
}



