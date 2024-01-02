#ifndef FAT_32_H
#define FAT_32_H

#include <stdint.h>
#include "common.h"
#include "memory.h"
#include "str_manip.h"
#include "hdd.h"

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

typedef enum {
    ExFAT,
    FAT12,
    FAT16,
    FAT32
 } fat_type;

extern BootSector boot_sector;
extern fat_type fatType;
typedef uint8_t bool;

void initialize_fat_file_system(FatFileSystem* fs, char* file);
void read_boot_sector(BootSector* bs);
uint32_t find_directory_entry(DirectoryEntry* entry, char* filename);
void update_directory_entry(DirectoryEntry* entry, char* filename, char* extension, uint8_t attributes, uint32_t first_cluster, uint32_t file_size);
void write_directory_entry(DirectoryEntry* entry, char* filename);
void identify_fat_system(uint32_t total_clusters);
void write_file(char* filename, uint32_t* FAT, uint32_t file_size);
void read_file(char* filename, char* buffer, uint32_t buffer_size);
void parse_filename(char* filename, char* name, char* ext);
void delete_file(char* filename);
void create_file(char *filename, FatFileSystem* fs, uint32_t* FAT);

// void copy_file
// void rename_file

#endif 