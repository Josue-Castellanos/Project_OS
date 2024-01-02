#ifndef HDD_H
#define HDD_H

#include <stdint.h>
#include "fat_32.h"


uint32_t getNextCluster(uint32_t current_cluster);

void write_sector(uint32_t sector_number, char* buffer, uint32_t sector_size);

void write_cluster(uint32_t cluster, char* buffer, uint32_t buffer_size);

void read_sector(uint32_t sector_number, char* buffer, uint32_t sector_size);

uint32_t find_free_cluster(uint32_t* FAT);

void free_clusters(uint32_t start_cluster);

void read_cluster(uint32_t cluster, char* buffer, uint32_t buffer_size);

uint32_t allocate_cluster();

void update_cluster(uint32_t start_cluster);

uint32_t extractLittleEndian32(uint8_t* buffer, uint32_t offset);


#endif