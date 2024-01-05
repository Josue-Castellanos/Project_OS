#ifndef HDD_H
#define HDD_H
#include <stdint.h>
#include "fat_32.h"

void write_sector(uint32_t sector_number, char* buffer, uint32_t sector_size);

void write_cluster(uint32_t cluster, char* buffer, uint32_t buffer_size);

void read_sector(uint32_t sector_number, char* buffer, uint32_t sector_size);

void read_cluster(uint32_t cluster, char* buffer, uint32_t buffer_size);


void update_cluster(uint32_t start_cluster);

uint32_t getNextCluster(uint32_t current_cluster);

uint32_t extractLittleEndian32(uint8_t* buffer, uint32_t offset);


uint32_t allocate_cluster();

uint32_t get_fat_entry(uint32_t cluster);

void set_fat_entry(uint32_t cluster, uint32_t value);

void read_fat_entry(uint32_t cluster, uint32_t* value);

void write_fat_entry(uint32_t cluster, uint32_t value);

void clear_cluster_data(uint32_t cluster);

#endif