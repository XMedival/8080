#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 8080 flags register: S Z 0 AC 0 P 1 C
typedef union {
    uint8_t byte;
    struct {
        uint8_t c  : 1;  // Carry
        uint8_t _1 : 1;  // Always 1
        uint8_t p  : 1;  // Parity
        uint8_t _0a: 1;  // Always 0
        uint8_t ac : 1;  // Auxiliary Carry
        uint8_t _0b: 1;  // Always 0
        uint8_t z  : 1;  // Zero
        uint8_t s  : 1;  // Sign
    };
} flags_t;

// Register pair union for easy hi/lo access
typedef union {
    uint16_t word;
    struct {
        uint8_t lo;
        uint8_t hi;
    };
} regpair_t;

typedef struct {
    // Accumulator and flags (PSW)
    uint8_t a;
    flags_t f;

    // Register pairs
    regpair_t bc;
    regpair_t de;
    regpair_t hl;

    // Special registers
    uint16_t sp;
    uint16_t pc;

    // CPU state
    bool halted;
    bool inte;  // Interrupt enable
    uint8_t int_pending;

    // Cycle counter
    uint64_t cycles;
} cpu_8080_t;

// Initialize CPU to reset state
void cpu_init(cpu_8080_t *cpu);

// Execute one instruction, returns cycles consumed
int cpu_step(cpu_8080_t *cpu);

// Raise an interrupt (RST 0-7)
void cpu_interrupt(cpu_8080_t *cpu, uint8_t rst_num);

// Debug: disassemble instruction at addr
int cpu_disasm(uint16_t addr, char *buf, size_t buf_size);

#endif
