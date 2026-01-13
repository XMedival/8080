#include "cpu.h"
#include "memory.h"
#include "io.h"
#include <stdio.h>

// Cycle counts for each opcode
static const uint8_t CYCLES[256] = {
//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4,  // 0
    4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4,  // 1
    4, 10, 16,  5,  5,  5,  7,  4,  4, 10, 16,  5,  5,  5,  7,  4,  // 2
    4, 10, 13,  5, 10, 10, 10,  4,  4, 10, 13,  5,  5,  5,  7,  4,  // 3
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 4
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 5
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 6
    7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,  // 7
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 8
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 9
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // A
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // B
    5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11,  // C
    5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11,  // D
    5, 10, 10, 18, 11, 11,  7, 11,  5,  5, 10,  4, 11, 17,  7, 11,  // E
    5, 10, 10,  4, 11, 11,  7, 11,  5,  5, 10,  4, 11, 17,  7, 11   // F
};

// Set Z, S, P flags based on result
#define SET_ZSP(cpu, val) do { \
    (cpu)->f.z = ((val) == 0); \
    (cpu)->f.s = ((val) >> 7) & 1; \
    (cpu)->f.p = parity(val); \
} while(0)

// Parity: 1 if even number of 1-bits
static inline bool parity(uint8_t val) {
    uint8_t p = val ^ (val >> 4);
    p ^= p >> 2;
    p ^= p >> 1;
    return !(p & 1);
}

// Carry helper for add operations
static inline bool carry(int bit, uint8_t a, uint8_t b, bool cy) {
    int16_t res = a + b + cy;
    int16_t c = res ^ a ^ b;
    return (c >> bit) & 1;
}

// Fetch next byte from PC
static inline uint8_t fetch(cpu_8080_t *cpu) {
    return mem_read(cpu->pc++);
}

// Fetch next word (little-endian)
static inline uint16_t fetch_word(cpu_8080_t *cpu) {
    uint16_t lo = mem_read(cpu->pc++);
    uint16_t hi = mem_read(cpu->pc++);
    return (hi << 8) | lo;
}

// Stack operations
static inline void push(cpu_8080_t *cpu, uint16_t val) {
    cpu->sp -= 2;
    mem_write(cpu->sp, val & 0xFF);
    mem_write(cpu->sp + 1, val >> 8);
}

static inline uint16_t pop(cpu_8080_t *cpu) {
    uint16_t val = mem_read(cpu->sp) | (mem_read(cpu->sp + 1) << 8);
    cpu->sp += 2;
    return val;
}

// ALU operations
static inline void alu_add(cpu_8080_t *cpu, uint8_t val, bool cy) {
    uint8_t res = cpu->a + val + cy;
    cpu->f.c = carry(8, cpu->a, val, cy);
    cpu->f.ac = carry(4, cpu->a, val, cy);
    SET_ZSP(cpu, res);
    cpu->a = res;
}

static inline void alu_sub(cpu_8080_t *cpu, uint8_t val, bool cy) {
    alu_add(cpu, ~val, !cy);
    cpu->f.c = !cpu->f.c;
}

static inline void alu_ana(cpu_8080_t *cpu, uint8_t val) {
    uint8_t res = cpu->a & val;
    cpu->f.c = 0;
    cpu->f.ac = ((cpu->a | val) >> 3) & 1;
    SET_ZSP(cpu, res);
    cpu->a = res;
}

static inline void alu_xra(cpu_8080_t *cpu, uint8_t val) {
    cpu->a ^= val;
    cpu->f.c = 0;
    cpu->f.ac = 0;
    SET_ZSP(cpu, cpu->a);
}

static inline void alu_ora(cpu_8080_t *cpu, uint8_t val) {
    cpu->a |= val;
    cpu->f.c = 0;
    cpu->f.ac = 0;
    SET_ZSP(cpu, cpu->a);
}

static inline void alu_cmp(cpu_8080_t *cpu, uint8_t val) {
    int16_t res = cpu->a - val;
    cpu->f.c = (res >> 8) & 1;
    cpu->f.ac = ~(cpu->a ^ res ^ val) & 0x10 ? 1 : 0;
    SET_ZSP(cpu, res & 0xFF);
}

static inline uint8_t alu_inr(cpu_8080_t *cpu, uint8_t val) {
    uint8_t res = val + 1;
    cpu->f.ac = (res & 0x0F) == 0;
    SET_ZSP(cpu, res);
    return res;
}

static inline uint8_t alu_dcr(cpu_8080_t *cpu, uint8_t val) {
    uint8_t res = val - 1;
    cpu->f.ac = (res & 0x0F) != 0x0F;
    SET_ZSP(cpu, res);
    return res;
}

static inline void alu_dad(cpu_8080_t *cpu, uint16_t val) {
    uint32_t res = cpu->hl.word + val;
    cpu->f.c = (res >> 16) & 1;
    cpu->hl.word = res & 0xFFFF;
}

// Rotate operations
static inline void alu_rlc(cpu_8080_t *cpu) {
    cpu->f.c = cpu->a >> 7;
    cpu->a = (cpu->a << 1) | cpu->f.c;
}

static inline void alu_rrc(cpu_8080_t *cpu) {
    cpu->f.c = cpu->a & 1;
    cpu->a = (cpu->a >> 1) | (cpu->f.c << 7);
}

static inline void alu_ral(cpu_8080_t *cpu) {
    uint8_t cy = cpu->f.c;
    cpu->f.c = cpu->a >> 7;
    cpu->a = (cpu->a << 1) | cy;
}

static inline void alu_rar(cpu_8080_t *cpu) {
    uint8_t cy = cpu->f.c;
    cpu->f.c = cpu->a & 1;
    cpu->a = (cpu->a >> 1) | (cy << 7);
}

// DAA - Decimal Adjust Accumulator
static inline void alu_daa(cpu_8080_t *cpu) {
    uint8_t cy = cpu->f.c;
    uint8_t correction = 0;

    if (cpu->f.ac || (cpu->a & 0x0F) > 9) {
        correction = 0x06;
    }
    if (cpu->f.c || cpu->a > 0x99 || ((cpu->a > 0x89) && (cpu->a & 0x0F) > 9)) {
        correction |= 0x60;
        cy = 1;
    }
    alu_add(cpu, correction, 0);
    cpu->f.c = cy;
}

// Jump/call/ret helpers
static inline void do_jmp(cpu_8080_t *cpu, uint16_t addr) {
    cpu->pc = addr;
}

static inline void cond_jmp(cpu_8080_t *cpu, bool cond) {
    uint16_t addr = fetch_word(cpu);
    if (cond) cpu->pc = addr;
}

static inline void do_call(cpu_8080_t *cpu, uint16_t addr) {
    push(cpu, cpu->pc);
    cpu->pc = addr;
}

static inline void cond_call(cpu_8080_t *cpu, bool cond) {
    uint16_t addr = fetch_word(cpu);
    if (cond) {
        do_call(cpu, addr);
        cpu->cycles += 6;
    }
}

static inline void do_ret(cpu_8080_t *cpu) {
    cpu->pc = pop(cpu);
}

static inline void cond_ret(cpu_8080_t *cpu, bool cond) {
    if (cond) {
        do_ret(cpu);
        cpu->cycles += 6;
    }
}

// PSW operations
static inline void push_psw(cpu_8080_t *cpu) {
    cpu->f._1 = 1;  // bit 1 always 1
    cpu->f._0a = 0; // bit 3 always 0
    cpu->f._0b = 0; // bit 5 always 0
    push(cpu, (cpu->a << 8) | cpu->f.byte);
}

static inline void pop_psw(cpu_8080_t *cpu) {
    uint16_t af = pop(cpu);
    cpu->a = af >> 8;
    cpu->f.byte = af & 0xFF;
}

void cpu_init(cpu_8080_t *cpu) {
    cpu->a = 0;
    cpu->f.byte = 0x02;  // bit 1 always 1
    cpu->bc.word = 0;
    cpu->de.word = 0;
    cpu->hl.word = 0;
    cpu->sp = 0;
    cpu->pc = 0;
    cpu->halted = false;
    cpu->inte = false;
    cpu->int_pending = 0;
    cpu->cycles = 0;
}

int cpu_step(cpu_8080_t *cpu) {
    if (cpu->halted) return 4;

    uint8_t op = fetch(cpu);
    cpu->cycles += CYCLES[op];

    switch (op) {
    // NOP
    case 0x00: case 0x08: case 0x10: case 0x18:
    case 0x20: case 0x28: case 0x30: case 0x38: break;

    // MOV r,r' - all 64 combinations
    case 0x40: cpu->bc.hi = cpu->bc.hi; break;  // MOV B,B
    case 0x41: cpu->bc.hi = cpu->bc.lo; break;  // MOV B,C
    case 0x42: cpu->bc.hi = cpu->de.hi; break;  // MOV B,D
    case 0x43: cpu->bc.hi = cpu->de.lo; break;  // MOV B,E
    case 0x44: cpu->bc.hi = cpu->hl.hi; break;  // MOV B,H
    case 0x45: cpu->bc.hi = cpu->hl.lo; break;  // MOV B,L
    case 0x46: cpu->bc.hi = mem_read(cpu->hl.word); break;  // MOV B,M
    case 0x47: cpu->bc.hi = cpu->a; break;      // MOV B,A

    case 0x48: cpu->bc.lo = cpu->bc.hi; break;  // MOV C,B
    case 0x49: cpu->bc.lo = cpu->bc.lo; break;  // MOV C,C
    case 0x4A: cpu->bc.lo = cpu->de.hi; break;  // MOV C,D
    case 0x4B: cpu->bc.lo = cpu->de.lo; break;  // MOV C,E
    case 0x4C: cpu->bc.lo = cpu->hl.hi; break;  // MOV C,H
    case 0x4D: cpu->bc.lo = cpu->hl.lo; break;  // MOV C,L
    case 0x4E: cpu->bc.lo = mem_read(cpu->hl.word); break;  // MOV C,M
    case 0x4F: cpu->bc.lo = cpu->a; break;      // MOV C,A

    case 0x50: cpu->de.hi = cpu->bc.hi; break;  // MOV D,B
    case 0x51: cpu->de.hi = cpu->bc.lo; break;  // MOV D,C
    case 0x52: cpu->de.hi = cpu->de.hi; break;  // MOV D,D
    case 0x53: cpu->de.hi = cpu->de.lo; break;  // MOV D,E
    case 0x54: cpu->de.hi = cpu->hl.hi; break;  // MOV D,H
    case 0x55: cpu->de.hi = cpu->hl.lo; break;  // MOV D,L
    case 0x56: cpu->de.hi = mem_read(cpu->hl.word); break;  // MOV D,M
    case 0x57: cpu->de.hi = cpu->a; break;      // MOV D,A

    case 0x58: cpu->de.lo = cpu->bc.hi; break;  // MOV E,B
    case 0x59: cpu->de.lo = cpu->bc.lo; break;  // MOV E,C
    case 0x5A: cpu->de.lo = cpu->de.hi; break;  // MOV E,D
    case 0x5B: cpu->de.lo = cpu->de.lo; break;  // MOV E,E
    case 0x5C: cpu->de.lo = cpu->hl.hi; break;  // MOV E,H
    case 0x5D: cpu->de.lo = cpu->hl.lo; break;  // MOV E,L
    case 0x5E: cpu->de.lo = mem_read(cpu->hl.word); break;  // MOV E,M
    case 0x5F: cpu->de.lo = cpu->a; break;      // MOV E,A

    case 0x60: cpu->hl.hi = cpu->bc.hi; break;  // MOV H,B
    case 0x61: cpu->hl.hi = cpu->bc.lo; break;  // MOV H,C
    case 0x62: cpu->hl.hi = cpu->de.hi; break;  // MOV H,D
    case 0x63: cpu->hl.hi = cpu->de.lo; break;  // MOV H,E
    case 0x64: cpu->hl.hi = cpu->hl.hi; break;  // MOV H,H
    case 0x65: cpu->hl.hi = cpu->hl.lo; break;  // MOV H,L
    case 0x66: cpu->hl.hi = mem_read(cpu->hl.word); break;  // MOV H,M
    case 0x67: cpu->hl.hi = cpu->a; break;      // MOV H,A

    case 0x68: cpu->hl.lo = cpu->bc.hi; break;  // MOV L,B
    case 0x69: cpu->hl.lo = cpu->bc.lo; break;  // MOV L,C
    case 0x6A: cpu->hl.lo = cpu->de.hi; break;  // MOV L,D
    case 0x6B: cpu->hl.lo = cpu->de.lo; break;  // MOV L,E
    case 0x6C: cpu->hl.lo = cpu->hl.hi; break;  // MOV L,H
    case 0x6D: cpu->hl.lo = cpu->hl.lo; break;  // MOV L,L
    case 0x6E: cpu->hl.lo = mem_read(cpu->hl.word); break;  // MOV L,M
    case 0x6F: cpu->hl.lo = cpu->a; break;      // MOV L,A

    case 0x70: mem_write(cpu->hl.word, cpu->bc.hi); break;  // MOV M,B
    case 0x71: mem_write(cpu->hl.word, cpu->bc.lo); break;  // MOV M,C
    case 0x72: mem_write(cpu->hl.word, cpu->de.hi); break;  // MOV M,D
    case 0x73: mem_write(cpu->hl.word, cpu->de.lo); break;  // MOV M,E
    case 0x74: mem_write(cpu->hl.word, cpu->hl.hi); break;  // MOV M,H
    case 0x75: mem_write(cpu->hl.word, cpu->hl.lo); break;  // MOV M,L
    case 0x77: mem_write(cpu->hl.word, cpu->a); break;      // MOV M,A

    case 0x78: cpu->a = cpu->bc.hi; break;  // MOV A,B
    case 0x79: cpu->a = cpu->bc.lo; break;  // MOV A,C
    case 0x7A: cpu->a = cpu->de.hi; break;  // MOV A,D
    case 0x7B: cpu->a = cpu->de.lo; break;  // MOV A,E
    case 0x7C: cpu->a = cpu->hl.hi; break;  // MOV A,H
    case 0x7D: cpu->a = cpu->hl.lo; break;  // MOV A,L
    case 0x7E: cpu->a = mem_read(cpu->hl.word); break;  // MOV A,M
    case 0x7F: cpu->a = cpu->a; break;      // MOV A,A

    // MVI r,d8
    case 0x06: cpu->bc.hi = fetch(cpu); break;  // MVI B
    case 0x0E: cpu->bc.lo = fetch(cpu); break;  // MVI C
    case 0x16: cpu->de.hi = fetch(cpu); break;  // MVI D
    case 0x1E: cpu->de.lo = fetch(cpu); break;  // MVI E
    case 0x26: cpu->hl.hi = fetch(cpu); break;  // MVI H
    case 0x2E: cpu->hl.lo = fetch(cpu); break;  // MVI L
    case 0x36: mem_write(cpu->hl.word, fetch(cpu)); break;  // MVI M
    case 0x3E: cpu->a = fetch(cpu); break;      // MVI A

    // LXI rp,d16
    case 0x01: cpu->bc.word = fetch_word(cpu); break;  // LXI B
    case 0x11: cpu->de.word = fetch_word(cpu); break;  // LXI D
    case 0x21: cpu->hl.word = fetch_word(cpu); break;  // LXI H
    case 0x31: cpu->sp = fetch_word(cpu); break;       // LXI SP

    // LDA/STA/LHLD/SHLD
    case 0x3A: cpu->a = mem_read(fetch_word(cpu)); break;  // LDA
    case 0x32: mem_write(fetch_word(cpu), cpu->a); break;  // STA
    case 0x2A: cpu->hl.word = mem_read16(fetch_word(cpu)); break;  // LHLD
    case 0x22: mem_write16(fetch_word(cpu), cpu->hl.word); break;  // SHLD

    // LDAX/STAX
    case 0x0A: cpu->a = mem_read(cpu->bc.word); break;  // LDAX B
    case 0x1A: cpu->a = mem_read(cpu->de.word); break;  // LDAX D
    case 0x02: mem_write(cpu->bc.word, cpu->a); break;  // STAX B
    case 0x12: mem_write(cpu->de.word, cpu->a); break;  // STAX D

    // XCHG, XTHL, SPHL
    case 0xEB: { uint16_t t = cpu->hl.word; cpu->hl.word = cpu->de.word; cpu->de.word = t; } break;  // XCHG
    case 0xE3: { uint16_t t = mem_read16(cpu->sp); mem_write16(cpu->sp, cpu->hl.word); cpu->hl.word = t; } break;  // XTHL
    case 0xF9: cpu->sp = cpu->hl.word; break;  // SPHL

    // ADD r
    case 0x80: alu_add(cpu, cpu->bc.hi, 0); break;
    case 0x81: alu_add(cpu, cpu->bc.lo, 0); break;
    case 0x82: alu_add(cpu, cpu->de.hi, 0); break;
    case 0x83: alu_add(cpu, cpu->de.lo, 0); break;
    case 0x84: alu_add(cpu, cpu->hl.hi, 0); break;
    case 0x85: alu_add(cpu, cpu->hl.lo, 0); break;
    case 0x86: alu_add(cpu, mem_read(cpu->hl.word), 0); break;
    case 0x87: alu_add(cpu, cpu->a, 0); break;
    case 0xC6: alu_add(cpu, fetch(cpu), 0); break;  // ADI

    // ADC r
    case 0x88: alu_add(cpu, cpu->bc.hi, cpu->f.c); break;
    case 0x89: alu_add(cpu, cpu->bc.lo, cpu->f.c); break;
    case 0x8A: alu_add(cpu, cpu->de.hi, cpu->f.c); break;
    case 0x8B: alu_add(cpu, cpu->de.lo, cpu->f.c); break;
    case 0x8C: alu_add(cpu, cpu->hl.hi, cpu->f.c); break;
    case 0x8D: alu_add(cpu, cpu->hl.lo, cpu->f.c); break;
    case 0x8E: alu_add(cpu, mem_read(cpu->hl.word), cpu->f.c); break;
    case 0x8F: alu_add(cpu, cpu->a, cpu->f.c); break;
    case 0xCE: alu_add(cpu, fetch(cpu), cpu->f.c); break;  // ACI

    // SUB r
    case 0x90: alu_sub(cpu, cpu->bc.hi, 0); break;
    case 0x91: alu_sub(cpu, cpu->bc.lo, 0); break;
    case 0x92: alu_sub(cpu, cpu->de.hi, 0); break;
    case 0x93: alu_sub(cpu, cpu->de.lo, 0); break;
    case 0x94: alu_sub(cpu, cpu->hl.hi, 0); break;
    case 0x95: alu_sub(cpu, cpu->hl.lo, 0); break;
    case 0x96: alu_sub(cpu, mem_read(cpu->hl.word), 0); break;
    case 0x97: alu_sub(cpu, cpu->a, 0); break;
    case 0xD6: alu_sub(cpu, fetch(cpu), 0); break;  // SUI

    // SBB r
    case 0x98: alu_sub(cpu, cpu->bc.hi, cpu->f.c); break;
    case 0x99: alu_sub(cpu, cpu->bc.lo, cpu->f.c); break;
    case 0x9A: alu_sub(cpu, cpu->de.hi, cpu->f.c); break;
    case 0x9B: alu_sub(cpu, cpu->de.lo, cpu->f.c); break;
    case 0x9C: alu_sub(cpu, cpu->hl.hi, cpu->f.c); break;
    case 0x9D: alu_sub(cpu, cpu->hl.lo, cpu->f.c); break;
    case 0x9E: alu_sub(cpu, mem_read(cpu->hl.word), cpu->f.c); break;
    case 0x9F: alu_sub(cpu, cpu->a, cpu->f.c); break;
    case 0xDE: alu_sub(cpu, fetch(cpu), cpu->f.c); break;  // SBI

    // ANA r
    case 0xA0: alu_ana(cpu, cpu->bc.hi); break;
    case 0xA1: alu_ana(cpu, cpu->bc.lo); break;
    case 0xA2: alu_ana(cpu, cpu->de.hi); break;
    case 0xA3: alu_ana(cpu, cpu->de.lo); break;
    case 0xA4: alu_ana(cpu, cpu->hl.hi); break;
    case 0xA5: alu_ana(cpu, cpu->hl.lo); break;
    case 0xA6: alu_ana(cpu, mem_read(cpu->hl.word)); break;
    case 0xA7: alu_ana(cpu, cpu->a); break;
    case 0xE6: alu_ana(cpu, fetch(cpu)); break;  // ANI

    // XRA r
    case 0xA8: alu_xra(cpu, cpu->bc.hi); break;
    case 0xA9: alu_xra(cpu, cpu->bc.lo); break;
    case 0xAA: alu_xra(cpu, cpu->de.hi); break;
    case 0xAB: alu_xra(cpu, cpu->de.lo); break;
    case 0xAC: alu_xra(cpu, cpu->hl.hi); break;
    case 0xAD: alu_xra(cpu, cpu->hl.lo); break;
    case 0xAE: alu_xra(cpu, mem_read(cpu->hl.word)); break;
    case 0xAF: alu_xra(cpu, cpu->a); break;
    case 0xEE: alu_xra(cpu, fetch(cpu)); break;  // XRI

    // ORA r
    case 0xB0: alu_ora(cpu, cpu->bc.hi); break;
    case 0xB1: alu_ora(cpu, cpu->bc.lo); break;
    case 0xB2: alu_ora(cpu, cpu->de.hi); break;
    case 0xB3: alu_ora(cpu, cpu->de.lo); break;
    case 0xB4: alu_ora(cpu, cpu->hl.hi); break;
    case 0xB5: alu_ora(cpu, cpu->hl.lo); break;
    case 0xB6: alu_ora(cpu, mem_read(cpu->hl.word)); break;
    case 0xB7: alu_ora(cpu, cpu->a); break;
    case 0xF6: alu_ora(cpu, fetch(cpu)); break;  // ORI

    // CMP r
    case 0xB8: alu_cmp(cpu, cpu->bc.hi); break;
    case 0xB9: alu_cmp(cpu, cpu->bc.lo); break;
    case 0xBA: alu_cmp(cpu, cpu->de.hi); break;
    case 0xBB: alu_cmp(cpu, cpu->de.lo); break;
    case 0xBC: alu_cmp(cpu, cpu->hl.hi); break;
    case 0xBD: alu_cmp(cpu, cpu->hl.lo); break;
    case 0xBE: alu_cmp(cpu, mem_read(cpu->hl.word)); break;
    case 0xBF: alu_cmp(cpu, cpu->a); break;
    case 0xFE: alu_cmp(cpu, fetch(cpu)); break;  // CPI

    // INR r
    case 0x04: cpu->bc.hi = alu_inr(cpu, cpu->bc.hi); break;
    case 0x0C: cpu->bc.lo = alu_inr(cpu, cpu->bc.lo); break;
    case 0x14: cpu->de.hi = alu_inr(cpu, cpu->de.hi); break;
    case 0x1C: cpu->de.lo = alu_inr(cpu, cpu->de.lo); break;
    case 0x24: cpu->hl.hi = alu_inr(cpu, cpu->hl.hi); break;
    case 0x2C: cpu->hl.lo = alu_inr(cpu, cpu->hl.lo); break;
    case 0x34: mem_write(cpu->hl.word, alu_inr(cpu, mem_read(cpu->hl.word))); break;
    case 0x3C: cpu->a = alu_inr(cpu, cpu->a); break;

    // DCR r
    case 0x05: cpu->bc.hi = alu_dcr(cpu, cpu->bc.hi); break;
    case 0x0D: cpu->bc.lo = alu_dcr(cpu, cpu->bc.lo); break;
    case 0x15: cpu->de.hi = alu_dcr(cpu, cpu->de.hi); break;
    case 0x1D: cpu->de.lo = alu_dcr(cpu, cpu->de.lo); break;
    case 0x25: cpu->hl.hi = alu_dcr(cpu, cpu->hl.hi); break;
    case 0x2D: cpu->hl.lo = alu_dcr(cpu, cpu->hl.lo); break;
    case 0x35: mem_write(cpu->hl.word, alu_dcr(cpu, mem_read(cpu->hl.word))); break;
    case 0x3D: cpu->a = alu_dcr(cpu, cpu->a); break;

    // INX/DCX rp
    case 0x03: cpu->bc.word++; break;
    case 0x13: cpu->de.word++; break;
    case 0x23: cpu->hl.word++; break;
    case 0x33: cpu->sp++; break;
    case 0x0B: cpu->bc.word--; break;
    case 0x1B: cpu->de.word--; break;
    case 0x2B: cpu->hl.word--; break;
    case 0x3B: cpu->sp--; break;

    // DAD rp
    case 0x09: alu_dad(cpu, cpu->bc.word); break;
    case 0x19: alu_dad(cpu, cpu->de.word); break;
    case 0x29: alu_dad(cpu, cpu->hl.word); break;
    case 0x39: alu_dad(cpu, cpu->sp); break;

    // Rotate
    case 0x07: alu_rlc(cpu); break;  // RLC
    case 0x0F: alu_rrc(cpu); break;  // RRC
    case 0x17: alu_ral(cpu); break;  // RAL
    case 0x1F: alu_rar(cpu); break;  // RAR

    // Misc
    case 0x27: alu_daa(cpu); break;  // DAA
    case 0x2F: cpu->a = ~cpu->a; break;  // CMA
    case 0x37: cpu->f.c = 1; break;  // STC
    case 0x3F: cpu->f.c = !cpu->f.c; break;  // CMC

    // JMP
    case 0xC3: case 0xCB: do_jmp(cpu, fetch_word(cpu)); break;
    case 0xC2: cond_jmp(cpu, !cpu->f.z); break;  // JNZ
    case 0xCA: cond_jmp(cpu, cpu->f.z); break;   // JZ
    case 0xD2: cond_jmp(cpu, !cpu->f.c); break;  // JNC
    case 0xDA: cond_jmp(cpu, cpu->f.c); break;   // JC
    case 0xE2: cond_jmp(cpu, !cpu->f.p); break;  // JPO
    case 0xEA: cond_jmp(cpu, cpu->f.p); break;   // JPE
    case 0xF2: cond_jmp(cpu, !cpu->f.s); break;  // JP
    case 0xFA: cond_jmp(cpu, cpu->f.s); break;   // JM
    case 0xE9: cpu->pc = cpu->hl.word; break;    // PCHL

    // CALL
    case 0xCD: case 0xDD: case 0xED: case 0xFD: do_call(cpu, fetch_word(cpu)); break;
    case 0xC4: cond_call(cpu, !cpu->f.z); break;  // CNZ
    case 0xCC: cond_call(cpu, cpu->f.z); break;   // CZ
    case 0xD4: cond_call(cpu, !cpu->f.c); break;  // CNC
    case 0xDC: cond_call(cpu, cpu->f.c); break;   // CC
    case 0xE4: cond_call(cpu, !cpu->f.p); break;  // CPO
    case 0xEC: cond_call(cpu, cpu->f.p); break;   // CPE
    case 0xF4: cond_call(cpu, !cpu->f.s); break;  // CP
    case 0xFC: cond_call(cpu, cpu->f.s); break;   // CM

    // RET
    case 0xC9: case 0xD9: do_ret(cpu); break;
    case 0xC0: cond_ret(cpu, !cpu->f.z); break;  // RNZ
    case 0xC8: cond_ret(cpu, cpu->f.z); break;   // RZ
    case 0xD0: cond_ret(cpu, !cpu->f.c); break;  // RNC
    case 0xD8: cond_ret(cpu, cpu->f.c); break;   // RC
    case 0xE0: cond_ret(cpu, !cpu->f.p); break;  // RPO
    case 0xE8: cond_ret(cpu, cpu->f.p); break;   // RPE
    case 0xF0: cond_ret(cpu, !cpu->f.s); break;  // RP
    case 0xF8: cond_ret(cpu, cpu->f.s); break;   // RM

    // RST n
    case 0xC7: do_call(cpu, 0x00); break;
    case 0xCF: do_call(cpu, 0x08); break;
    case 0xD7: do_call(cpu, 0x10); break;
    case 0xDF: do_call(cpu, 0x18); break;
    case 0xE7: do_call(cpu, 0x20); break;
    case 0xEF: do_call(cpu, 0x28); break;
    case 0xF7: do_call(cpu, 0x30); break;
    case 0xFF: do_call(cpu, 0x38); break;

    // PUSH/POP
    case 0xC5: push(cpu, cpu->bc.word); break;
    case 0xD5: push(cpu, cpu->de.word); break;
    case 0xE5: push(cpu, cpu->hl.word); break;
    case 0xF5: push_psw(cpu); break;
    case 0xC1: cpu->bc.word = pop(cpu); break;
    case 0xD1: cpu->de.word = pop(cpu); break;
    case 0xE1: cpu->hl.word = pop(cpu); break;
    case 0xF1: pop_psw(cpu); break;

    // I/O
    case 0xDB: cpu->a = io_read(fetch(cpu)); break;  // IN
    case 0xD3: io_write(fetch(cpu), cpu->a); break;  // OUT

    // Interrupts
    case 0xF3: cpu->inte = false; break;  // DI
    case 0xFB: cpu->inte = true; break;   // EI

    // HLT
    case 0x76: cpu->halted = true; break;
    }

    return CYCLES[op];
}

void cpu_interrupt(cpu_8080_t *cpu, uint8_t rst_num) {
    if (cpu->inte) {
        cpu->inte = false;
        cpu->halted = false;
        do_call(cpu, rst_num * 8);
        cpu->cycles += 11;
    }
}

// Instruction lengths: 1, 2, or 3 bytes
static const uint8_t OP_LENGTHS[256] = {
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 0
    1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 1
    1, 3, 3, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 2, 1,  // 2
    1, 3, 3, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 2, 1,  // 3
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 4
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 5
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 6
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 7
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 8
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 9
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // A
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // B
    1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 3, 3, 3, 2, 1,  // C
    1, 1, 3, 2, 3, 1, 2, 1, 1, 1, 3, 2, 3, 3, 2, 1,  // D
    1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1,  // E
    1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1   // F
};

int cpu_disasm(uint16_t addr, char *buf, size_t buf_size) {
    static const char *MNEMONICS[256] = {
        "NOP", "LXI B,", "STAX B", "INX B", "INR B", "DCR B", "MVI B,", "RLC",
        "NOP", "DAD B", "LDAX B", "DCX B", "INR C", "DCR C", "MVI C,", "RRC",
        "NOP", "LXI D,", "STAX D", "INX D", "INR D", "DCR D", "MVI D,", "RAL",
        "NOP", "DAD D", "LDAX D", "DCX D", "INR E", "DCR E", "MVI E,", "RAR",
        "NOP", "LXI H,", "SHLD ", "INX H", "INR H", "DCR H", "MVI H,", "DAA",
        "NOP", "DAD H", "LHLD ", "DCX H", "INR L", "DCR L", "MVI L,", "CMA",
        "NOP", "LXI SP,", "STA ", "INX SP", "INR M", "DCR M", "MVI M,", "STC",
        "NOP", "DAD SP", "LDA ", "DCX SP", "INR A", "DCR A", "MVI A,", "CMC",
        "MOV B,B", "MOV B,C", "MOV B,D", "MOV B,E", "MOV B,H", "MOV B,L", "MOV B,M", "MOV B,A",
        "MOV C,B", "MOV C,C", "MOV C,D", "MOV C,E", "MOV C,H", "MOV C,L", "MOV C,M", "MOV C,A",
        "MOV D,B", "MOV D,C", "MOV D,D", "MOV D,E", "MOV D,H", "MOV D,L", "MOV D,M", "MOV D,A",
        "MOV E,B", "MOV E,C", "MOV E,D", "MOV E,E", "MOV E,H", "MOV E,L", "MOV E,M", "MOV E,A",
        "MOV H,B", "MOV H,C", "MOV H,D", "MOV H,E", "MOV H,H", "MOV H,L", "MOV H,M", "MOV H,A",
        "MOV L,B", "MOV L,C", "MOV L,D", "MOV L,E", "MOV L,H", "MOV L,L", "MOV L,M", "MOV L,A",
        "MOV M,B", "MOV M,C", "MOV M,D", "MOV M,E", "MOV M,H", "MOV M,L", "HLT", "MOV M,A",
        "MOV A,B", "MOV A,C", "MOV A,D", "MOV A,E", "MOV A,H", "MOV A,L", "MOV A,M", "MOV A,A",
        "ADD B", "ADD C", "ADD D", "ADD E", "ADD H", "ADD L", "ADD M", "ADD A",
        "ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC M", "ADC A",
        "SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB M", "SUB A",
        "SBB B", "SBB C", "SBB D", "SBB E", "SBB H", "SBB L", "SBB M", "SBB A",
        "ANA B", "ANA C", "ANA D", "ANA E", "ANA H", "ANA L", "ANA M", "ANA A",
        "XRA B", "XRA C", "XRA D", "XRA E", "XRA H", "XRA L", "XRA M", "XRA A",
        "ORA B", "ORA C", "ORA D", "ORA E", "ORA H", "ORA L", "ORA M", "ORA A",
        "CMP B", "CMP C", "CMP D", "CMP E", "CMP H", "CMP L", "CMP M", "CMP A",
        "RNZ", "POP B", "JNZ ", "JMP ", "CNZ ", "PUSH B", "ADI ", "RST 0",
        "RZ", "RET", "JZ ", "JMP ", "CZ ", "CALL ", "ACI ", "RST 1",
        "RNC", "POP D", "JNC ", "OUT ", "CNC ", "PUSH D", "SUI ", "RST 2",
        "RC", "RET", "JC ", "IN ", "CC ", "CALL ", "SBI ", "RST 3",
        "RPO", "POP H", "JPO ", "XTHL", "CPO ", "PUSH H", "ANI ", "RST 4",
        "RPE", "PCHL", "JPE ", "XCHG", "CPE ", "CALL ", "XRI ", "RST 5",
        "RP", "POP PSW", "JP ", "DI", "CP ", "PUSH PSW", "ORI ", "RST 6",
        "RM", "SPHL", "JM ", "EI", "CM ", "CALL ", "CPI ", "RST 7"
    };

    uint8_t op = mem_read(addr);
    uint8_t len = OP_LENGTHS[op];

    if (len == 1) {
        snprintf(buf, buf_size, "%s", MNEMONICS[op]);
    } else if (len == 2) {
        uint8_t byte = mem_read(addr + 1);
        snprintf(buf, buf_size, "%s%02Xh", MNEMONICS[op], byte);
    } else {
        uint16_t word = mem_read(addr + 1) | (mem_read(addr + 2) << 8);
        snprintf(buf, buf_size, "%s%04Xh", MNEMONICS[op], word);
    }

    return len;
}
