#ifndef CRC_H
#define CRC_H

/*
 * Calculate CRC32 of memory region
 */
unsigned long crc32_calculate(const unsigned char *data, size_t len);

#endif
