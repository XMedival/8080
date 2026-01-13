#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define MEMORY_SIZE 65536

// Initialize memory (zeroes it out)
void mem_init(void);

// Read/write single byte
uint8_t mem_read(uint16_t addr);
void mem_write(uint16_t addr, uint8_t val);

// Read/write 16-bit word (little endian)
uint16_t mem_read16(uint16_t addr);
void mem_write16(uint16_t addr, uint16_t val);

// Load data into memory at specified address
void mem_load(uint16_t addr, const uint8_t *data, size_t len);

// Get pointer to memory (for DMA-style access)
uint8_t *mem_get_ptr(uint16_t addr);

#endif
