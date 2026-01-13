#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "cpu.h"
#include "memory.h"
#include "io.h"
#include "panel.h"

// Set to 1 when you have the LED panel connected
#define PANEL_ENABLED 1

static cpu_8080_t cpu;

// Parse hex number from string
static uint16_t parse_hex(const char *s) {
    uint16_t val = 0;
    while (*s) {
        char c = toupper(*s++);
        if (c >= '0' && c <= '9') val = (val << 4) | (c - '0');
        else if (c >= 'A' && c <= 'F') val = (val << 4) | (c - 'A' + 10);
        else break;
    }
    return val;
}

// Parse exactly n hex digits
static uint16_t parse_hex_n(const char *s, int n) {
    uint16_t val = 0;
    for (int i = 0; i < n && s[i]; i++) {
        char c = toupper(s[i]);
        if (c >= '0' && c <= '9') val = (val << 4) | (c - '0');
        else if (c >= 'A' && c <= 'F') val = (val << 4) | (c - 'A' + 10);
    }
    return val;
}

// Print CPU state
static void print_state(void) {
    printf("A=%02X BC=%04X DE=%04X HL=%04X SP=%04X PC=%04X\n",
           cpu.a, cpu.bc.word, cpu.de.word, cpu.hl.word, cpu.sp, cpu.pc);
    printf("Flags: %c%c%c%c%c  INTE=%d  CYC=%llu\n",
           cpu.f.s ? 'S' : '-',
           cpu.f.z ? 'Z' : '-',
           cpu.f.ac ? 'A' : '-',
           cpu.f.p ? 'P' : '-',
           cpu.f.c ? 'C' : '-',
           cpu.inte,
           cpu.cycles);
}

// Dump memory
static void dump_mem(uint16_t addr, uint16_t len) {
    for (uint16_t i = 0; i < len; i += 16) {
        printf("%04X: ", addr + i);
        for (int j = 0; j < 16 && (i + j) < len; j++) {
            printf("%02X ", mem_read(addr + i + j));
        }
        printf("\n");
    }
}

// Simple monitor
static void monitor(void) {
    static char line[64];
    static int pos = 0;

    printf("\nAltair 8800 Emulator\n");
    printf("Commands:\n");
    printf("  r        - Run until HLT or keypress\n");
    printf("  s        - Single step\n");
    printf("  g addr   - Set PC to address\n");
    printf("  e addr b - Enter bytes at address\n");
    printf("  d addr n - Dump n bytes at address\n");
    printf("  u addr n - Disassemble n instructions\n");
    printf("  l        - Load Intel HEX\n");
    printf("  ?        - Show CPU state\n");
    printf("  x        - Reset CPU and memory\n");
    printf("> ");

    while (1) {
        int c = getchar_timeout_us(100);
        if (c == PICO_ERROR_TIMEOUT) continue;

        if (c == '\r' || c == '\n') {
            line[pos] = 0;
            putchar('\n');

            if (pos > 0) {
                char cmd = tolower(line[0]);
                char *args = &line[1];
                while (*args == ' ') args++;

                switch (cmd) {
                case 'r':  // Run
                    printf("Running... (any key to stop)\n");
                    cpu.halted = false;
                    front_panel.run = true;
                    while (!cpu.halted) {
                        cpu_step(&cpu);
                        front_panel.address_display = cpu.pc;
                        front_panel.data_display = mem_read(cpu.pc);
#if PANEL_ENABLED
                        panel_update_leds();
                        panel_read_switches();
                        if (panel_get_control_press() & SW_STOP) break;
#endif
                        if (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) break;
                    }
                    front_panel.run = false;
                    print_state();
                    break;

                case 's':  // Step
                    cpu_step(&cpu);
                    front_panel.address_display = cpu.pc;
                    front_panel.data_display = mem_read(cpu.pc);
#if PANEL_ENABLED
                    panel_update_leds();
#endif
                    print_state();
                    break;

                case 'g': {  // Go to address
                    uint16_t addr = parse_hex(args);
                    cpu.pc = addr;
                    printf("PC=%04X\n", cpu.pc);
                    break;
                }

                case 'e': {  // Enter bytes at address
                    uint16_t addr = parse_hex(args);
                    while (*args && !isspace(*args)) args++;
                    while (*args) {
                        while (*args == ' ') args++;
                        if (*args) {
                            uint8_t val = parse_hex(args);
                            mem_write(addr++, val);
                            while (*args && !isspace(*args)) args++;
                        }
                    }
                    printf("OK\n");
                    break;
                }

                case 'd': {  // Dump memory
                    uint16_t addr = parse_hex(args);
                    while (*args && !isspace(*args)) args++;
                    while (*args == ' ') args++;
                    uint16_t len = *args ? parse_hex(args) : 64;
                    dump_mem(addr, len);
                    break;
                }

                case 'u': {  // Disassemble
                    uint16_t addr = *args ? parse_hex(args) : cpu.pc;
                    while (*args && !isspace(*args)) args++;
                    while (*args == ' ') args++;
                    int count = *args ? parse_hex(args) : 10;
                    char disasm_buf[32];
                    for (int i = 0; i < count; i++) {
                        int len = cpu_disasm(addr, disasm_buf, sizeof(disasm_buf));
                        printf("%04X: ", addr);
                        for (int j = 0; j < 3; j++) {
                            if (j < len) printf("%02X ", mem_read(addr + j));
                            else printf("   ");
                        }
                        printf(" %s\n", disasm_buf);
                        addr += len;
                    }
                    break;
                }

                case '?':  // Show state
                    print_state();
                    break;

                case 'x':  // Reset
                    cpu_init(&cpu);
                    mem_init();
                    printf("Reset\n");
                    break;

                case 'l': {  // Load Intel HEX
                    printf("Paste Intel HEX (end with EOF record or blank line):\n");
                    uint16_t total_bytes = 0;
                    uint16_t start_addr = 0xFFFF;
                    bool first = true;

                    while (1) {
                        // Read a line
                        char hexline[80];
                        int hpos = 0;
                        while (hpos < 79) {
                            int ch = getchar_timeout_us(5000000);  // 5s timeout
                            if (ch == PICO_ERROR_TIMEOUT) break;
                            if (ch == '\r' || ch == '\n') {
                                putchar('\n');
                                break;
                            }
                            hexline[hpos++] = ch;
                            putchar(ch);
                        }
                        hexline[hpos] = 0;

                        // Empty line = done
                        if (hpos == 0) break;

                        // Must start with ':'
                        if (hexline[0] != ':') {
                            printf("Error: line must start with ':'\n");
                            continue;
                        }

                        // Parse: :LLAAAATT[DD...]CC
                        uint8_t len = parse_hex_n(&hexline[1], 2);
                        uint16_t addr = parse_hex_n(&hexline[3], 4);
                        uint8_t type = parse_hex_n(&hexline[7], 2);

                        if (type == 0x01) {  // EOF record
                            printf("EOF record\n");
                            break;
                        }

                        if (type == 0x00) {  // Data record
                            if (first) {
                                start_addr = addr;
                                first = false;
                            }
                            for (int i = 0; i < len; i++) {
                                uint8_t byte = parse_hex_n(&hexline[9 + i * 2], 2);
                                mem_write(addr + i, byte);
                                total_bytes++;
                            }
                        }
                    }
                    printf("Loaded %d bytes", total_bytes);
                    if (start_addr != 0xFFFF) {
                        printf(" starting at %04Xh", start_addr);
                        cpu.pc = start_addr;
                    }
                    printf("\n");
                    break;
                }

                default:
                    printf("Unknown command\n");
                }
            }

            pos = 0;
            printf("> ");
        } else if (c == 127 || c == 8) {  // Backspace
            if (pos > 0) {
                pos--;
                printf("\b \b");
            }
        } else if (pos < sizeof(line) - 1) {
            line[pos++] = c;
            putchar(c);
        }
    }
}

int main() {
    stdio_init_all();

    // Wait for USB serial connection
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    mem_init();
    cpu_init(&cpu);
    io_init();
#if PANEL_ENABLED
    panel_init();
#endif

    // Load a simple test program at 0x0000
    // OUT 1, A  - output accumulator to serial
    // HLT
    static const uint8_t test_prog[] = {
        0x3E, 0x48,  // MVI A, 'H'
        0xD3, 0x01,  // OUT 1
        0x3E, 0x69,  // MVI A, 'i'
        0xD3, 0x01,  // OUT 1
        0x3E, 0x0A,  // MVI A, '\n'
        0xD3, 0x01,  // OUT 1
        0x76         // HLT
    };
    mem_load(0x0000, test_prog, sizeof(test_prog));

    monitor();

    return 0;
}
