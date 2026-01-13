// 8080 Assembler - outputs Intel HEX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_LABELS 1024
#define MAX_LINE 256

typedef struct {
    char name[32];
    uint16_t addr;
} Label;

static Label labels[MAX_LABELS];
static int label_count = 0;
static uint16_t current_addr = 0;
static int line_num = 0;
static int pass = 1;

// Output buffer for Intel HEX
static uint8_t hex_buf[16];
static int hex_count = 0;
static uint16_t hex_addr = 0;
static FILE *outfile = NULL;

// Opcode tables
typedef struct {
    const char *mnem;
    uint8_t opcode;
    int len;  // 1, 2, or 3 bytes
} Opcode;

static const Opcode simple_ops[] = {
    {"NOP", 0x00, 1}, {"HLT", 0x76, 1}, {"RET", 0xC9, 1},
    {"PCHL", 0xE9, 1}, {"SPHL", 0xF9, 1}, {"XCHG", 0xEB, 1},
    {"XTHL", 0xE3, 1}, {"EI", 0xFB, 1}, {"DI", 0xF3, 1},
    {"RLC", 0x07, 1}, {"RRC", 0x0F, 1}, {"RAL", 0x17, 1},
    {"RAR", 0x1F, 1}, {"DAA", 0x27, 1}, {"CMA", 0x2F, 1},
    {"STC", 0x37, 1}, {"CMC", 0x3F, 1},
    // Conditional returns
    {"RNZ", 0xC0, 1}, {"RZ", 0xC8, 1}, {"RNC", 0xD0, 1},
    {"RC", 0xD8, 1}, {"RPO", 0xE0, 1}, {"RPE", 0xE8, 1},
    {"RP", 0xF0, 1}, {"RM", 0xF8, 1},
    {NULL, 0, 0}
};

static int reg_num(char c) {
    switch (toupper(c)) {
        case 'B': return 0; case 'C': return 1;
        case 'D': return 2; case 'E': return 3;
        case 'H': return 4; case 'L': return 5;
        case 'M': return 6; case 'A': return 7;
        default: return -1;
    }
}

static int rp_num(const char *s) {
    if (toupper(s[0]) == 'B') return 0;
    if (toupper(s[0]) == 'D') return 1;
    if (toupper(s[0]) == 'H') return 2;
    if (strncasecmp(s, "SP", 2) == 0) return 3;
    if (strncasecmp(s, "PSW", 3) == 0) return 3;
    return -1;
}

static void add_label(const char *name) {
    if (label_count >= MAX_LABELS) {
        fprintf(stderr, "Error: too many labels\n");
        exit(1);
    }
    size_t len = strlen(name);
    if (len >= sizeof(labels[0].name)) len = sizeof(labels[0].name) - 1;
    memcpy(labels[label_count].name, name, len);
    labels[label_count].name[len] = '\0';
    labels[label_count].addr = current_addr;
    label_count++;
}

static int lookup_label(const char *name, uint16_t *addr) {
    for (int i = 0; i < label_count; i++) {
        if (strcasecmp(labels[i].name, name) == 0) {
            *addr = labels[i].addr;
            return 1;
        }
    }
    return 0;
}

static uint16_t parse_number(const char *s) {
    if (!s || !*s) return 0;

    int len = strlen(s);

    // Hex with H suffix
    if (toupper(s[len-1]) == 'H') {
        return (uint16_t)strtol(s, NULL, 16);
    }
    // Hex with 0x prefix
    if (len > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return (uint16_t)strtol(s + 2, NULL, 16);
    }
    // Binary with B suffix
    if (toupper(s[len-1]) == 'B' && s[0] != '0') {
        char tmp[32];
        int copylen = len - 1;
        if (copylen > 31) copylen = 31;
        memcpy(tmp, s, copylen);
        tmp[copylen] = '\0';
        return (uint16_t)strtol(tmp, NULL, 2);
    }
    // Character literal
    if (s[0] == '\'' && len >= 2) {
        return (uint8_t)s[1];
    }
    // Decimal
    return (uint16_t)atoi(s);
}

static int parse_operand(const char *s, uint16_t *val) {
    if (!s || !*s) return 0;

    // Is it a number?
    if (isdigit(s[0]) || s[0] == '\'' ||
        (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))) {
        *val = parse_number(s);
        return 1;
    }

    // Try label lookup
    if (lookup_label(s, val)) {
        return 1;
    }

    // On pass 1, assume unknown identifiers are forward references
    if (pass == 1) {
        *val = 0;
        return 1;
    }

    fprintf(stderr, "Line %d: undefined symbol '%s'\n", line_num, s);
    return 0;
}

static void flush_hex(void) {
    if (hex_count == 0) return;

    uint8_t checksum = hex_count + (hex_addr >> 8) + (hex_addr & 0xFF);
    fprintf(outfile, ":%02X%04X00", hex_count, hex_addr);
    for (int i = 0; i < hex_count; i++) {
        fprintf(outfile, "%02X", hex_buf[i]);
        checksum += hex_buf[i];
    }
    fprintf(outfile, "%02X\n", (uint8_t)(~checksum + 1));
    hex_count = 0;
}

static void emit_byte(uint8_t b) {
    if (pass == 1) {
        current_addr++;
        return;
    }

    if (hex_count == 0) {
        hex_addr = current_addr;
    }
    hex_buf[hex_count++] = b;
    current_addr++;

    if (hex_count >= 16) {
        flush_hex();
    }
}

static void emit_word(uint16_t w) {
    emit_byte(w & 0xFF);
    emit_byte(w >> 8);
}

static char *skip_ws(char *p) {
    while (*p && (*p == ' ' || *p == '\t')) p++;
    return p;
}

static char *get_token(char *p, char *buf, int buflen) {
    p = skip_ws(p);
    int i = 0;
    while (*p && !isspace(*p) && *p != ',' && *p != ';' && *p != ':' && i < buflen - 1) {
        buf[i++] = *p++;
    }
    buf[i] = '\0';
    return p;
}

static void process_line(char *line) {
    char label[32] = "", mnem[16] = "", op1[32] = "", op2[32] = "";
    char *p = line;

    line_num++;

    // Skip leading whitespace
    p = skip_ws(p);
    if (!*p || *p == ';' || *p == '*') return;

    // Check for label (identifier followed by colon)
    char *colon = strchr(p, ':');
    if (colon && colon > p) {
        // Make sure it's not inside a string or after mnemonic
        char *space = p;
        while (space < colon && !isspace(*space)) space++;
        if (space >= colon) {
            // It's a label
            int len = colon - p;
            if (len > 31) len = 31;
            strncpy(label, p, len);
            label[len] = '\0';
            if (pass == 1) {
                add_label(label);
            }
            p = colon + 1;
        }
    }

    // Get mnemonic
    p = get_token(p, mnem, sizeof(mnem));
    if (!mnem[0]) return;

    // Get operands
    p = skip_ws(p);
    if (*p && *p != ';') {
        p = get_token(p, op1, sizeof(op1));
        p = skip_ws(p);
        if (*p == ',') {
            p++;
            p = get_token(p, op2, sizeof(op2));
        }
    }

    // Convert mnemonic to uppercase
    for (int i = 0; mnem[i]; i++) mnem[i] = toupper(mnem[i]);

    // Directives
    if (strcmp(mnem, "ORG") == 0) {
        uint16_t addr;
        parse_operand(op1, &addr);
        if (pass == 2 && hex_count > 0) flush_hex();
        current_addr = addr;
        return;
    }

    if (strcmp(mnem, "DB") == 0 || strcmp(mnem, "DEFB") == 0) {
        uint16_t val;
        parse_operand(op1, &val);
        emit_byte(val & 0xFF);
        return;
    }

    if (strcmp(mnem, "DW") == 0 || strcmp(mnem, "DEFW") == 0) {
        uint16_t val;
        parse_operand(op1, &val);
        emit_word(val);
        return;
    }

    if (strcmp(mnem, "DS") == 0 || strcmp(mnem, "DEFS") == 0) {
        uint16_t count;
        parse_operand(op1, &count);
        for (int i = 0; i < count; i++) emit_byte(0);
        return;
    }

    if (strcmp(mnem, "EQU") == 0) {
        // For EQU, the label was already added with current_addr
        // We need to update it with the actual value
        if (pass == 1 && label[0]) {
            uint16_t val;
            parse_operand(op1, &val);
            // Update the last label's address
            labels[label_count - 1].addr = val;
        }
        return;
    }

    // Simple opcodes
    for (int i = 0; simple_ops[i].mnem; i++) {
        if (strcmp(mnem, simple_ops[i].mnem) == 0) {
            emit_byte(simple_ops[i].opcode);
            return;
        }
    }

    uint16_t val;
    int r, r2, rp;

    // MOV dst, src
    if (strcmp(mnem, "MOV") == 0) {
        r = reg_num(op1[0]);
        r2 = reg_num(op2[0]);
        if (r >= 0 && r2 >= 0) {
            emit_byte(0x40 | (r << 3) | r2);
            return;
        }
    }

    // MVI reg, imm8
    if (strcmp(mnem, "MVI") == 0) {
        r = reg_num(op1[0]);
        if (r >= 0 && parse_operand(op2, &val)) {
            emit_byte(0x06 | (r << 3));
            emit_byte(val & 0xFF);
            return;
        }
    }

    // LXI rp, imm16
    if (strcmp(mnem, "LXI") == 0) {
        rp = rp_num(op1);
        if (rp >= 0 && parse_operand(op2, &val)) {
            emit_byte(0x01 | (rp << 4));
            emit_word(val);
            return;
        }
    }

    // Register pair ops: PUSH, POP, DAD, INX, DCX
    if (strcmp(mnem, "PUSH") == 0) {
        rp = rp_num(op1);
        if (rp >= 0) { emit_byte(0xC5 | (rp << 4)); return; }
    }
    if (strcmp(mnem, "POP") == 0) {
        rp = rp_num(op1);
        if (rp >= 0) { emit_byte(0xC1 | (rp << 4)); return; }
    }
    if (strcmp(mnem, "DAD") == 0) {
        rp = rp_num(op1);
        if (rp >= 0) { emit_byte(0x09 | (rp << 4)); return; }
    }
    if (strcmp(mnem, "INX") == 0) {
        rp = rp_num(op1);
        if (rp >= 0) { emit_byte(0x03 | (rp << 4)); return; }
    }
    if (strcmp(mnem, "DCX") == 0) {
        rp = rp_num(op1);
        if (rp >= 0) { emit_byte(0x0B | (rp << 4)); return; }
    }
    if (strcmp(mnem, "LDAX") == 0) {
        rp = rp_num(op1);
        if (rp >= 0 && rp <= 1) { emit_byte(0x0A | (rp << 4)); return; }
    }
    if (strcmp(mnem, "STAX") == 0) {
        rp = rp_num(op1);
        if (rp >= 0 && rp <= 1) { emit_byte(0x02 | (rp << 4)); return; }
    }

    // ALU ops with register: ADD, ADC, SUB, SBB, ANA, XRA, ORA, CMP
    #define ALU_REG(name, base) \
        if (strcmp(mnem, name) == 0) { \
            r = reg_num(op1[0]); \
            if (r >= 0) { emit_byte(base | r); return; } \
        }
    ALU_REG("ADD", 0x80) ALU_REG("ADC", 0x88)
    ALU_REG("SUB", 0x90) ALU_REG("SBB", 0x98)
    ALU_REG("ANA", 0xA0) ALU_REG("XRA", 0xA8)
    ALU_REG("ORA", 0xB0) ALU_REG("CMP", 0xB8)

    // ALU ops with immediate: ADI, ACI, SUI, SBI, ANI, XRI, ORI, CPI
    #define ALU_IMM(name, op) \
        if (strcmp(mnem, name) == 0 && parse_operand(op1, &val)) { \
            emit_byte(op); emit_byte(val & 0xFF); return; \
        }
    ALU_IMM("ADI", 0xC6) ALU_IMM("ACI", 0xCE)
    ALU_IMM("SUI", 0xD6) ALU_IMM("SBI", 0xDE)
    ALU_IMM("ANI", 0xE6) ALU_IMM("XRI", 0xEE)
    ALU_IMM("ORI", 0xF6) ALU_IMM("CPI", 0xFE)

    // INR, DCR
    if (strcmp(mnem, "INR") == 0) {
        r = reg_num(op1[0]);
        if (r >= 0) { emit_byte(0x04 | (r << 3)); return; }
    }
    if (strcmp(mnem, "DCR") == 0) {
        r = reg_num(op1[0]);
        if (r >= 0) { emit_byte(0x05 | (r << 3)); return; }
    }

    // Jumps: JMP, JNZ, JZ, JNC, JC, JPO, JPE, JP, JM
    #define JUMP(name, op) \
        if (strcmp(mnem, name) == 0 && parse_operand(op1, &val)) { \
            emit_byte(op); emit_word(val); return; \
        }
    JUMP("JMP", 0xC3) JUMP("JNZ", 0xC2) JUMP("JZ", 0xCA)
    JUMP("JNC", 0xD2) JUMP("JC", 0xDA) JUMP("JPO", 0xE2)
    JUMP("JPE", 0xEA) JUMP("JP", 0xF2) JUMP("JM", 0xFA)

    // Calls: CALL, CNZ, CZ, CNC, CC, CPO, CPE, CP, CM
    JUMP("CALL", 0xCD) JUMP("CNZ", 0xC4) JUMP("CZ", 0xCC)
    JUMP("CNC", 0xD4) JUMP("CC", 0xDC) JUMP("CPO", 0xE4)
    JUMP("CPE", 0xEC) JUMP("CP", 0xF4) JUMP("CM", 0xFC)

    // LDA, STA, LHLD, SHLD
    JUMP("LDA", 0x3A) JUMP("STA", 0x32)
    JUMP("LHLD", 0x2A) JUMP("SHLD", 0x22)

    // IN, OUT
    if (strcmp(mnem, "IN") == 0 && parse_operand(op1, &val)) {
        emit_byte(0xDB); emit_byte(val & 0xFF); return;
    }
    if (strcmp(mnem, "OUT") == 0 && parse_operand(op1, &val)) {
        emit_byte(0xD3); emit_byte(val & 0xFF); return;
    }

    // RST n
    if (strcmp(mnem, "RST") == 0) {
        int n = op1[0] - '0';
        if (n >= 0 && n <= 7) {
            emit_byte(0xC7 | (n << 3));
            return;
        }
    }

    fprintf(stderr, "Line %d: unknown instruction '%s'\n", line_num, mnem);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input.asm [output.hex]\n", argv[0]);
        return 1;
    }

    const char *inname = argv[1];
    const char *outname = argc > 2 ? argv[2] : "out.hex";

    FILE *infile = fopen(inname, "r");
    if (!infile) {
        fprintf(stderr, "Error: cannot open %s\n", inname);
        return 1;
    }

    char line[MAX_LINE];

    // Pass 1: collect labels
    printf("Pass 1: collecting labels...\n");
    pass = 1;
    current_addr = 0;
    line_num = 0;
    while (fgets(line, sizeof(line), infile)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = '\0';
        process_line(line);
    }
    printf("Found %d labels\n", label_count);

    // Pass 2: generate code
    printf("Pass 2: generating code...\n");
    pass = 2;
    current_addr = 0;
    line_num = 0;
    rewind(infile);

    outfile = fopen(outname, "w");
    if (!outfile) {
        fprintf(stderr, "Error: cannot create %s\n", outname);
        fclose(infile);
        return 1;
    }

    while (fgets(line, sizeof(line), infile)) {
        line[strcspn(line, "\r\n")] = '\0';
        process_line(line);
    }

    flush_hex();
    fprintf(outfile, ":00000001FF\n");  // EOF record

    fclose(infile);
    fclose(outfile);

    printf("Output: %s\n", outname);
    return 0;
}
