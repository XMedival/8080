# Intel 8080 Emulator for Raspberry Pi Pico

A fully functional Intel 8080 emulator running on the Raspberry Pi Pico with USB serial communication and Altair 8800-style front panel support.

## Building

Requires the Raspberry Pi Pico SDK.

```bash
cd emulator
mkdir build && cd build
cmake ..
make
```

Flash the resulting `.uf2` file to your Pico.

## Operating the Emulator

Connect via USB serial (115200 baud). The emulator provides an interactive monitor with the following commands:

| Command | Description |
|---------|-------------|
| `r` | Run until HLT or keypress |
| `s` | Single step one instruction |
| `g addr` | Set program counter to address |
| `e addr b` | Enter byte(s) at address |
| `d addr n` | Dump n bytes starting at address |
| `u addr n` | Disassemble n instructions at address |
| `l` | Load Intel HEX format program |
| `?` | Show CPU state (registers, flags, PC) |
| `x` | Reset CPU and clear memory |

### Loading Programs

Use the `l` command to paste Intel HEX format code. The included COBOL compiler can assemble `.a80` files to Intel HEX format:

```bash
cd compiler
./compiler < test.a80 > program.hex
```

## I/O Ports

| Port | Name | Direction | Description |
|------|------|-----------|-------------|
| `0x00` | Serial Status | IN | Bit 0: RX ready, Bit 1: TX ready |
| `0x01` | Serial Data | IN/OUT | Read/write serial data via USB |
| `0xFE` | Sense Switches Lo | IN | Front panel switches (bits 0-7) |
| `0xFF` | Sense Switches Hi | IN | Front panel switches (bits 8-15) |

### Serial I/O Example

```asm
; Wait for TX ready, then send character
WAIT:   IN   0x00       ; Read status
        ANI  0x02       ; Check TX ready bit
        JZ   WAIT       ; Loop if not ready
        MVI  A, 'H'     ; Character to send
        OUT  0x01       ; Output to serial

; Check for received character
        IN   0x00       ; Read status
        ANI  0x01       ; Check RX ready bit
        JZ   NO_CHAR    ; Skip if no character
        IN   0x01       ; Read the character
```

## GPIO Pin Assignments

The front panel uses shift registers for LED output and switch input.

### Output (74HC595 Chain)

| GPIO | Function |
|------|----------|
| 2 | 595 Data (serial input) |
| 3 | 595 Clock |
| 4 | 595 Latch |

### Input (74HC165 Chain)

| GPIO | Function |
|------|----------|
| 5 | 165 Data (serial output) |
| 6 | 165 Clock |
| 7 | 165 Load (parallel capture) |

## Front Panel LED Chain

5x 74HC595 shift registers daisy-chained, outputting 40 bits (MSB first):

| Bits | Content |
|------|---------|
| 39-32 | Status High: INTE, PROT, MEMR, INP, M1, OUT, HLTA, STACK |
| 31-24 | Status Low: WO, INT, ... |
| 23-16 | Data bus (D7-D0) |
| 15-8 | Address High (A15-A8) |
| 7-0 | Address Low (A7-A0) |

### Status LED Bits

| Bit | Name | Description |
|-----|------|-------------|
| 0x80 | INTE | Interrupt Enable |
| 0x40 | PROT | Memory Protection |
| 0x20 | MEMR | Memory Read |
| 0x10 | INP | Input operation |
| 0x08 | M1 | Machine cycle 1 (opcode fetch) |
| 0x04 | OUT | Output operation |
| 0x02 | HLTA | Halt Acknowledge |
| 0x01 | STACK | Stack operation |

## Front Panel Switch Chain

3x 74HC165 shift registers, reading 24 bits (MSB first):

| Bits | Content |
|------|---------|
| 23-16 | Control switches |
| 15-8 | Sense switches high (SW15-SW8) |
| 7-0 | Sense switches low (SW7-SW0) |

### Control Switch Bits

| Bit | Name |
|-----|------|
| 0x80 | RESET |
| 0x40 | DEPOSIT NEXT |
| 0x20 | DEPOSIT |
| 0x10 | EXAMINE NEXT |
| 0x08 | EXAMINE |
| 0x04 | SINGLE STEP |
| 0x02 | RUN |
| 0x01 | STOP |

## Memory

- 64KB flat address space (0x0000 - 0xFFFF)
- Programs typically load at 0x0000
- Stack grows downward (initialize SP to top of RAM)

## CPU Features

Full Intel 8080 instruction set with accurate cycle timing:

- All register operations (MOV, MVI, LXI, etc.)
- Arithmetic (ADD, ADC, SUB, SBB, INR, DCR, DAD)
- Logical (ANA, XRA, ORA, CMP)
- Rotate (RLC, RRC, RAL, RAR)
- Jumps and calls (conditional and unconditional)
- Stack operations (PUSH, POP)
- I/O (IN, OUT)
- Interrupts (DI, EI, RST 0-7)

## Project Structure

```
emulator/src/
  main.c      - Monitor and initialization
  cpu.c       - 8080 instruction set implementation
  cpu.h       - CPU registers and types
  io.c        - I/O port handlers
  io.h        - Port definitions
  memory.c    - RAM management
  memory.h    - Memory interface
  panel.c     - Front panel driver
  panel.h     - GPIO and status definitions

compiler/
  compiler.cbl - COBOL assembler source
  compiler     - Compiled assembler
  test.a80     - Example assembly program
```
