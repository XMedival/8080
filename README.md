# Intel 8080 Emulator for Raspberry Pi Pico

An Intel 8080 emulator running on the Raspberry Pi Pico with USB serial and Altair 8800-style front panel support.

## Building

### Emulator

Requires the Raspberry Pi Pico SDK.

```bash
cd emulator
mkdir build && cd build
cmake ..
make
```

Flash the resulting `.uf2` file to your Pico.

### Assembler

```bash
cd compiler
make
```

## Assembler Usage

```bash
./asm8080 input.asm output.hex
```

### Supported Syntax

```asm
; Comments start with semicolon
        ORG 0           ; Set origin address

START:  MVI A, 'H'      ; Labels end with colon
        OUT 1           ; Output to port
        LXI H, 1000h    ; Hex numbers end with H
        MVI B, 10       ; Decimal numbers
        MVI C, 01010101B; Binary numbers end with B
        JMP START       ; Jump to label
        HLT

        DB 'A'          ; Define byte (character)
        DB 48h          ; Define byte (hex)
        DW 1234h        ; Define word
        DS 16           ; Define space (16 bytes)
COUNT:  EQU 10          ; Equate (constant)
```

### Full Instruction Set

All 8080 instructions supported: MOV, MVI, LXI, LDA, STA, LDAX, STAX, LHLD, SHLD, XCHG, ADD, ADC, SUB, SBB, INR, DCR, INX, DCX, DAD, ANA, ORA, XRA, CMP, ADI, ACI, SUI, SBI, ANI, ORI, XRI, CPI, RLC, RRC, RAL, RAR, JMP, Jcc, CALL, Ccc, RET, Rcc, RST, PUSH, POP, IN, OUT, EI, DI, HLT, NOP, PCHL, SPHL, XTHL, DAA, CMA, STC, CMC.

## Monitor Commands

Connect via USB serial (115200 baud):

| Command | Description |
|---------|-------------|
| `r` | Run until HLT or keypress |
| `s` | Single step |
| `g addr` | Set PC to address |
| `e addr b...` | Enter bytes at address |
| `d addr n` | Dump n bytes |
| `u addr n` | Disassemble n instructions |
| `l` | Load Intel HEX |
| `?` | Show CPU state |
| `x` | Reset CPU and memory |

### Loading Programs

```
> l
Paste Intel HEX (end with EOF record or blank line):
:0D0000003E48D3013E69D3013E0AD301768C
:00000001FF
Loaded 13 bytes starting at 0000h
> r
Hi
```

## I/O Ports

| Port | Direction | Description |
|------|-----------|-------------|
| 0x00 | IN | Serial status (bit 0: RX ready, bit 1: TX ready) |
| 0x01 | IN/OUT | Serial data |
| 0xFE | IN | Sense switches low (SW7-SW0) |
| 0xFF | IN | Sense switches high (SW15-SW8) |

## Front Panel

### GPIO Pins

| GPIO | Function |
|------|----------|
| 2 | 595 Data |
| 3 | 595 Clock |
| 4 | 595 Latch |
| 5 | 165 Data |
| 6 | 165 Clock |
| 7 | 165 Load |

### LED Chain (5x 74HC595)

40 bits, MSB first:
- Bits 39-32: Status (INTE, PROT, MEMR, INP, M1, OUT, HLTA, STACK)
- Bits 31-24: Status low
- Bits 23-16: Data bus (D7-D0)
- Bits 15-0: Address bus (A15-A0)

### Switch Chain (3x 74HC165)

24 bits, MSB first:
- Bits 23-16: Control (RESET, DEP NEXT, DEP, EXAM NEXT, EXAM, STEP, RUN, STOP)
- Bits 15-0: Sense switches (SW15-SW0)

## Project Structure

```
emulator/
  src/
    main.c    - Monitor, Intel HEX loader
    cpu.c/h   - 8080 CPU emulation
    memory.c/h- 64KB RAM
    io.c/h    - I/O port handlers
    panel.c/h - Front panel shift register driver
  CMakeLists.txt

compiler/
  asm8080.c - 8080 assembler (C)
  Makefile
```
