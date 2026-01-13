#include "memory.h"
#include <string.h>

static uint8_t ram[MEMORY_SIZE];

void mem_init(void) {
    memset(ram, 0, sizeof(ram));
}

uint8_t mem_read(uint16_t addr) {
    return ram[addr];
}

void mem_write(uint16_t addr, uint8_t val) {
    ram[addr] = val;
}

uint16_t mem_read16(uint16_t addr) {
    return ram[addr] | (ram[addr + 1] << 8);
}

void mem_write16(uint16_t addr, uint16_t val) {
    ram[addr] = val & 0xFF;
    ram[addr + 1] = val >> 8;
}

void mem_load(uint16_t addr, const uint8_t *data, size_t len) {
    memcpy(&ram[addr], data, len);
}

uint8_t *mem_get_ptr(uint16_t addr) {
    return &ram[addr];
}
