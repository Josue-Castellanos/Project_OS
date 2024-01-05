#include "fat_32.h"
#include "constants.h"
#include "vga.h"
#include "hdd.h"
#include "common.h"
#include "memory.h"
#include "strings.h"


fat_type fatType; 
BootSector boot_sector;



// This function reads the boot sector, detects the FAT type, and initializes the file system structure.
void initialize_fat_file_system(FatFileSystem* fs, char* file)
{
    print_set_color(GREEN, BLACK);
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
    print_str("Total Sectors: ");
    print_int (total_sectors);
    print_str("\n");
    // Total sectors per cluster
    print_str("Sectors per Cluster: ");
    print_int (fs->boot_sector.sectors_per_cluster);
    print_str("\n");
    // FAT size in sectors:
    uint32_t fat_size = fs->boot_sector.sectors_per_fat_32 * fs->boot_sector.bytes_per_sector;
    print_str("FAT Size (in sectors): ");
    print_int (fat_size);
    print_str("\n");
    // Sectors occupied by the root directory:
    uint32_t root_dir_sectors = ((fs->boot_sector.root_entry_count * 32) + (fs->boot_sector.bytes_per_sector - 1)) / fs->boot_sector.bytes_per_sector;
    print_str("Root Directory Sectors: ");
    print_int (root_dir_sectors);
    print_str("\n");
    // First data sector number:
    uint32_t first_data_sector = fs->boot_sector.reserved_sector_count + (fs->boot_sector.fat_count * fat_size) + root_dir_sectors;
    print_str("First Data Sector: ");
    print_int (first_data_sector);
    print_str("\n");
    // Total data sectors:
    uint32_t first_fat_sector = fs->boot_sector.reserved_sector_count;
    print_str("First FAT Sector: ");
    print_int (first_fat_sector);
    print_str("\n");
    // Total data sectors:
    uint32_t data_sectors = total_sectors - (fs->boot_sector.reserved_sector_count + (fs->boot_sector.fat_count * fat_size) + root_dir_sectors);
    print_str("Data Sectors: ");
    print_int (data_sectors);
    print_str("\n");
    // Total clusters in volume:
    uint32_t total_clusters = data_sectors / fs->boot_sector.sectors_per_cluster;
    print_str("Total Clusters: ");
    print_int (total_clusters);
    print_str("\n");

    print_set_color(YELLOW, BLACK);
    print_str("\nFile system initialized:\n");
    fs->boot_sector.total_clusters = total_clusters;
    identify_fat_system(fs->boot_sector.total_clusters);
}

//
void read_boot_sector(BootSector* bs)
{
    // Disk is memory-mapped at address 0x100000
    uint8_t *disk = (uint8_t *)0x100000;

    // Copy the boot sector into the structure
    memCpy(bs, disk, sizeof(BootSector));

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

// Identify fat system
void identify_fat_system(uint32_t total_clusters) {
    // Calculate the sector number of the first sector in the cluster
    if (SECTOR_SIZE == 0) {
        // This is an ExFAT file system
        print_str("Detected ExFAT\n");
        fatType = ExFAT;
    }
    else if (total_clusters < 4085)
    {
        // FAT12
        print_str("Detected FAT12\n");
        fatType = FAT12;
    }
    else if (total_clusters < 65525)
    {
        // FAT16
        print_str("Detected FAT16\n");
        fatType = FAT16;
    }
    else
    {
        // FAT32
        print_str("Detected FAT32\n");
        fatType = FAT32;
    }
}

// write file
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

// read_file
void read_file(char* filename, char* buffer, uint32_t buffer_size) {

    char* name[FILENAME_LENGTH + 1];
    char* ext[EXTENSION_LENGTH + 1];

    // Parse the filename and extension
    //parse_filename(filename, name, ext);

    // Find the directory entry for the file
    DirectoryEntry newEntry;
    if (find_directory_entry(&newEntry, filename) == 0) {
        // Replace this with your actual VGA print function
        print_str("\nFile not found\n");
        return;
    }

    // Read the content of the file
    read_cluster(newEntry.cluster_low | (newEntry.cluster_high << 16), buffer, buffer_size);

    print_str("\nData read completed\n");
    print_str("\nFile content:\n");
    print_str(buffer);
    print_str("\n");
    buffer[newEntry.file_size] = '\0';

}

void read_cluster(uint32_t cluster, char* buffer, uint32_t buffer_size) {
    read_boot_sector(&boot_sector);

    // Calculate the sector number of the first sector in the cluster
    uint32_t first_sector_of_cluster = ((cluster - 2) * boot_sector.sectors_per_cluster) + boot_sector.reserved_sector_count + (boot_sector.fat_count * boot_sector.sectors_per_fat_32) + ((boot_sector.root_entry_count * 32) + (boot_sector.bytes_per_sector - 1)) / boot_sector.bytes_per_sector;

    // Read the cluster into a buffer
    read_sector(first_sector_of_cluster, buffer, buffer_size);
}

// Parse the filename and extension
void parse_filename(char* filename, char* name, char* ext) {
    // Copy the filename
    memCpy(name, filename, FILENAME_LENGTH);

    // Find the dot separator
    char* dot = strChr(name, '.');

    if (dot != NULL) {
        // Copy the extension
        memCpy(ext, dot + 1, EXTENSION_LENGTH);

        // Replace the dot with a null terminator
        *dot = '\0';
    }
    else {
        // No extension found, set the extension to an empty string
        ext[0] = '\0';
    }
}

void create_file(char* filename, FatFileSystem* fs) {
    print_set_color(RED, BLACK);
    read_boot_sector(&fs->boot_sector);

    char name[FILENAME_LENGTH + 1];
    char ext[EXTENSION_LENGTH + 1];

    // Parse the filename and extension
    parse_filename(filename, name, ext);

    // Sectors occupied by the root directory:
    uint32_t root_dir_sectors = ((fs->boot_sector.root_entry_count * 32) + (fs->boot_sector.bytes_per_sector - 1)) / fs->boot_sector.bytes_per_sector;

    uint32_t fat_size = fs->boot_sector.sectors_per_fat_32 * fs->boot_sector.bytes_per_sector;

    // First data sector number:
    uint32_t first_data_sector = fs->boot_sector.reserved_sector_count + (fs->boot_sector.fat_count * fat_size) + root_dir_sectors;

    // Total data sectors:
    uint32_t first_fat_sector = fs->boot_sector.reserved_sector_count;

    char buff[fs->boot_sector.bytes_per_sector];
    read_sector(first_data_sector, buff, fs->boot_sector.bytes_per_sector);

    // Find an empty directory entry (first byte of filename will be 0x00)
    for (int i = 0; i < fs->boot_sector.bytes_per_sector; i += 64)
    {
        if (buff[i] == '\0')
        {
            // Find an available cluster
            uint32_t first_cluster = allocate_cluster();

            // If no available cluster was found, print an error message and return
            if (first_cluster == 0)
            {
                print_str("\nNo available clusters found\n");
                print_str("\n");
                return;
            }

            DirectoryEntry entry;
            // Copy the filename and extension
            memCpy(entry.filename, filename, FILENAME_LENGTH);
            memCpy(entry.ext, ext, EXTENSION_LENGTH);

            // Set the file attributes
            entry.attributes = 0x20;

            // Set the file size
            *(uint32_t *)&entry.file_size = FILE_SIZE;

            // Set the first cluster
            *(uint16_t *)&entry.cluster_low = first_cluster;

            // Write the directory entry back to the buffer
            memCpy(buff + i, &entry, sizeof(DirectoryEntry));

            // Write the sector back to disk
            write_sector(first_data_sector, buff, fs->boot_sector.bytes_per_sector);

            // Update the directory entry
            update_directory_entry(&entry, name, ext, 0x20, first_cluster, 0);

            return;
        }
    }
    print_str("\nNo empty directory entries found\n");
} 
