#include "io.h"
#include <stdio.h>
#include "pico/stdlib.h"

front_panel_t front_panel = {0};

static uint8_t serial_in_buf = 0;
static bool serial_in_ready = false;

void io_init(void) {
    front_panel.address_display = 0;
    front_panel.data_display = 0;
    front_panel.sense_switches = 0;
    front_panel.run = false;
    front_panel.wait = false;
}

uint8_t io_read(uint8_t port) {
    switch (port) {
    case PORT_SERIAL_STATUS: {
        uint8_t status = 0;
        // Check if USB serial has data available
        int ch = getchar_timeout_us(0);
        if (ch != PICO_ERROR_TIMEOUT) {
            serial_in_buf = ch;
            serial_in_ready = true;
        }
        if (serial_in_ready) status |= 0x01;  // RX ready
        status |= 0x02;  // TX always ready
        return status;
    }

    case PORT_SERIAL_DATA:
        if (serial_in_ready) {
            serial_in_ready = false;
            return serial_in_buf;
        }
        return 0;

    case PORT_SENSE_SW_HI:
        return front_panel.sense_switches >> 8;

    case PORT_SENSE_SW_LO:
        return front_panel.sense_switches & 0xFF;

    default:
        return 0xFF;
    }
}

void io_write(uint8_t port, uint8_t val) {
    switch (port) {
    case PORT_SERIAL_DATA:
        putchar(val);
        break;

    default:
        break;
    }

    // Update front panel data display
    front_panel.data_display = val;
}

bool io_serial_available(void) {
    if (serial_in_ready) return true;
    int ch = getchar_timeout_us(0);
    if (ch != PICO_ERROR_TIMEOUT) {
        serial_in_buf = ch;
        serial_in_ready = true;
        return true;
    }
    return false;
}
