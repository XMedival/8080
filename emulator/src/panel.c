#include "panel.h"
#include "io.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Debounce state
static uint8_t last_control = 0;
static uint8_t control_pressed = 0;
static uint32_t debounce_time = 0;

void panel_init(void) {
    // 595 outputs
    gpio_init(PIN_595_DATA);
    gpio_init(PIN_595_CLOCK);
    gpio_init(PIN_595_LATCH);
    gpio_set_dir(PIN_595_DATA, GPIO_OUT);
    gpio_set_dir(PIN_595_CLOCK, GPIO_OUT);
    gpio_set_dir(PIN_595_LATCH, GPIO_OUT);

    // 165 inputs
    gpio_init(PIN_165_DATA);
    gpio_init(PIN_165_CLOCK);
    gpio_init(PIN_165_LOAD);
    gpio_set_dir(PIN_165_DATA, GPIO_IN);
    gpio_set_dir(PIN_165_CLOCK, GPIO_OUT);
    gpio_set_dir(PIN_165_LOAD, GPIO_OUT);

    // Initial states
    gpio_put(PIN_595_DATA, 0);
    gpio_put(PIN_595_CLOCK, 0);
    gpio_put(PIN_595_LATCH, 0);
    gpio_put(PIN_165_CLOCK, 0);
    gpio_put(PIN_165_LOAD, 1);  // Active low, keep high

    // Clear all LEDs
    panel_update_leds();
}

// Shift out a single byte, MSB first
static inline void shift_out_byte(uint8_t val) {
    for (int i = 7; i >= 0; i--) {
        gpio_put(PIN_595_DATA, (val >> i) & 1);
        gpio_put(PIN_595_CLOCK, 1);
        gpio_put(PIN_595_CLOCK, 0);
    }
}

void panel_update_leds(void) {
    // Build status byte from CPU state
    uint8_t status_hi = 0;
    uint8_t status_lo = 0;

    if (front_panel.run) status_hi |= ST_MEMR;
    if (front_panel.wait) status_hi |= ST_HLTA;
    // Add more status bits as needed from CPU state

    // Shift out 40 bits: status_hi, status_lo, data, addr_hi, addr_lo
    shift_out_byte(status_hi);
    shift_out_byte(status_lo);
    shift_out_byte(front_panel.data_display);
    shift_out_byte(front_panel.address_display >> 8);
    shift_out_byte(front_panel.address_display & 0xFF);

    // Latch outputs
    gpio_put(PIN_595_LATCH, 1);
    gpio_put(PIN_595_LATCH, 0);
}

// Shift in a single byte, MSB first
static inline uint8_t shift_in_byte(void) {
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) {
        val <<= 1;
        val |= gpio_get(PIN_165_DATA) ? 1 : 0;
        gpio_put(PIN_165_CLOCK, 1);
        gpio_put(PIN_165_CLOCK, 0);
    }
    return val;
}

void panel_read_switches(void) {
    // Pulse load low to capture parallel inputs
    gpio_put(PIN_165_LOAD, 0);
    gpio_put(PIN_165_LOAD, 1);

    // Shift in 24 bits: control, sense_hi, sense_lo
    uint8_t control = shift_in_byte();
    uint8_t sense_hi = shift_in_byte();
    uint8_t sense_lo = shift_in_byte();

    front_panel.sense_switches = (sense_hi << 8) | sense_lo;

    // Debounce control switches (detect rising edge)
    uint32_t now = time_us_32();
    if (now - debounce_time > 50000) {  // 50ms debounce
        uint8_t newly_pressed = control & ~last_control;
        if (newly_pressed) {
            control_pressed = newly_pressed;
            debounce_time = now;
        }
    }
    last_control = control;
}

uint8_t panel_get_control_press(void) {
    uint8_t pressed = control_pressed;
    control_pressed = 0;
    return pressed;
}
