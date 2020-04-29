#ifndef _FAT32_H_
#define _FAT32_H_

#include <stdint.h>
#include <stdbool.h>

bool fat32_read(uint8_t *b, uint32_t addr);
bool fat32_write(uint8_t *b, uint32_t addr);

#endif
