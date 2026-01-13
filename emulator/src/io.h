#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdbool.h>

// Altair 8800 I/O ports
// Port 0x00: Serial status (bit 0 = rx ready, bit 1 = tx ready)
// Port 0x01: Serial data

#define PORT_SERIAL_STATUS  0x00
#define PORT_SERIAL_DATA    0x01

// Front panel ports (directly accessible)
#define PORT_SENSE_SW_HI    0xFF  // Sense switches high byte
#define PORT_SENSE_SW_LO    0xFE  // Sense switches low byte

// Initialize I/O subsystem
void io_init(void);

// Read from I/O port
uint8_t io_read(uint8_t port);

// Write to I/O port
void io_write(uint8_t port, uint8_t val);

// Check if serial input available (for polling)
bool io_serial_available(void);

// Front panel state (for future LED panel)
typedef struct {
    uint16_t address_display;
    uint8_t data_display;
    uint16_t sense_switches;
    bool run;
    bool wait;
} front_panel_t;

extern front_panel_t front_panel;

#endif
