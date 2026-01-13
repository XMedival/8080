#ifndef PANEL_H
#define PANEL_H

#include <stdint.h>
#include <stdbool.h>

/*
ALTAIR 8800 FRONT PANEL
=======================

The Pico has 26 GPIO. The Altair panel needs ~60 I/O lines.
Solution: Shift registers
  - 74HC595 (serial-in, parallel-out) for LEDs
  - 74HC165 (parallel-in, serial-out) for switches

GPIO PINS
---------
  GPIO 2 : 595 Data  (directly to all 595 SER pins daisy-chained)
  GPIO 3 : 595 Clock (to all SRCLK)
  GPIO 4 : 595 Latch (to all RCLK)
  GPIO 5 : 165 Data  (from last 165 QH)
  GPIO 6 : 165 Clock (to all CLK)
  GPIO 7 : 165 Load  (to all /PL, directly low to load)

LED CHAIN (5x 74HC595, shifted data exits last chip)
---------------------------------------------------
  Pico GPIO2 -> 595#1 SER -> 595#2 SER -> 595#3 SER -> 595#4 SER -> 595#5 SER
               (Stat Hi)    (Stat Lo)    (Data)      (Addr Hi)    (Addr Lo)

  GPIO3 -> all SRCLK (directly together)
  GPIO4 -> all RCLK  (directly together)

  Shift 40 bits MSB first: Status[15:8], Status[7:0], Data[7:0], Addr[15:8], Addr[7:0]

SWITCH CHAIN (3x 74HC165)
-------------------------
  165#1 QH -> 165#2 DS -> 165#2 QH -> 165#3 DS -> 165#3 QH -> GPIO5
  (Sw Lo)    (Sw Hi)      (Control)

  GPIO6 -> all CLK
  GPIO7 -> all /PL

  Pulse /PL low, then clock 24 bits. First bit = Control bit 7.

74HC595 PINOUT
--------------
       +---u---+
  QB  1|       |16 VCC -- 3.3V
  QC  2|       |15 QA  -- LED anode (cathode via 220R to GND)
  QD  3|       |14 SER -- GPIO2 or prev QH'
  QE  4|  595  |13 /OE -- GND
  QF  5|       |12 RCLK - GPIO4
  QG  6|       |11 SRCLK GPIO3
  QH  7|       |10 /SRCLR 3.3V
 GND  8|       |9  QH' -- next SER
       +-------+

74HC165 PINOUT
--------------
       +---u---+
 /PL  1|       |16 VCC -- 3.3V
 CLK  2|       |15 /CE -- GND
  D4  3|       |14 D3
  D5  4|  165  |13 D2
  D6  5|       |12 D1
  D7  6|       |11 D0
 /QH  7|       |10 DS  -- prev QH or GND
 GND  8|       |9  QH  -- GPIO5 or next DS
       +-------+

SWITCH ACCENT (accent high when closed)
---------------------------------------
  3.3V --[SW]--+-- 165 Dx
               |
             [10K]
               |
              GND

BIT MAP
-------
LED out (40 bits):
  [39:32] Status hi: INTE PROT MEMR INP M1 OUT HLTA STACK
  [31:24] Status lo: WO INT x x x x x x
  [23:16] Data: D7-D0
  [15:8]  Addr: A15-A8
  [7:0]   Addr: A7-A0

Switch in (24 bits):
  [23:16] Control: RESET DEP_NXT DEP EX_NXT EXAM STEP RUN STOP
  [15:8]  Sense: SW15-SW8
  [7:0]   Sense: SW7-SW0
*/

// GPIO
#define PIN_595_DATA    2
#define PIN_595_CLOCK   3
#define PIN_595_LATCH   4
#define PIN_165_DATA    5
#define PIN_165_CLOCK   6
#define PIN_165_LOAD    7

// Control switch masks
#define SW_STOP         0x01
#define SW_RUN          0x02
#define SW_SINGLE_STEP  0x04
#define SW_EXAMINE      0x08
#define SW_EXAMINE_NEXT 0x10
#define SW_DEPOSIT      0x20
#define SW_DEPOSIT_NEXT 0x40
#define SW_RESET        0x80

// Status bits
#define ST_INTE   0x80
#define ST_PROT   0x40
#define ST_MEMR   0x20
#define ST_INP    0x10
#define ST_M1     0x08
#define ST_OUT    0x04
#define ST_HLTA   0x02
#define ST_STACK  0x01

void panel_init(void);
void panel_update_leds(void);
void panel_read_switches(void);
uint8_t panel_get_control_press(void);

#endif
