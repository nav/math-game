# Hardware Wiring — Physical Device (Stage H1-H3)

This covers the electrical wiring for the arcade Confirm button, the two
PN532 NFC pads, and the WS2812 LED strip described in
`IMPLEMENTATION_PLAN.md`'s "Physical Hardware Platform" section and Stages
H1-H3. None of this is required to run the game today — the current build
uses keyboard input only (Stage 0/1 implementation) — this is the reference
for when the enclosure is wired up.

**This board is a Raspberry Pi Model B *Revision 1*, not Revision 2.**
Revision 1 has a 26-pin header (not the later 40-pin one) and, importantly,
two pins differ from every later Pi: physical pins 3 and 5 are `GPIO0`/
`GPIO1` (I2C0), not `GPIO2`/`GPIO3` (I2C1), and physical pin 13 is `GPIO21`,
not `GPIO27`. Double-check any pinout diagram you find online is the
Rev 1 one, not the far more common Rev 2/later diagrams — most "Raspberry Pi
1 GPIO pinout" search results are Rev 2. (This wiring plan doesn't use pins
3/5/13 at all, so it's a non-issue here, but matters if you extend this later.)

## Pin assignment summary

| Physical pin | BCM GPIO | Function | Connects to |
|---|---|---|---|
| 1 | — (3.3V) | Power | PN532 reader 1 (tens pad) VCC |
| 17 | — (3.3V) | Power | PN532 reader 2 (ones pad) VCC |
| 6 | — (GND) | Ground | PN532 reader 1 GND |
| 9 | — (GND) | Ground | PN532 reader 2 GND |
| 14 | — (GND) | Ground | Common ground tie to external LED strip supply |
| 19 | GPIO10 | SPI0 MOSI | Both PN532 readers' MOSI (shared bus) |
| 21 | GPIO9 | SPI0 MISO | Both PN532 readers' MISO (shared bus) |
| 23 | GPIO11 | SPI0 SCLK | Both PN532 readers' SCK (shared bus) |
| 24 | GPIO8 | SPI0 CE0 | PN532 reader 1 (tens pad) SS/CS |
| 26 | GPIO7 | SPI0 CE1 | PN532 reader 2 (ones pad) SS/CS |
| 12 | GPIO18 | PWM0 | WS2812 LED strip DIN (via series resistor) |
| 11 | GPIO17 | GPIO | Confirm button (other leg to GND) |

Everything else on the header (I2C0 on pins 3/5, UART on pins 8/10, and the
spare GPIOs 4/22/23/24/25) is unused and free for future buttons.

## ASCII pinout (Rev 1, 26-pin P1 header)

Looking at the header with the USB ports facing down, pin 1 is top-left:

```
                 3.3V  [ 1] [ 2]  5V
   PN532 #1 VCC ----+          |
   PN532 #2 VCC ----|----------+---- (pin 17, below)
       I2C0 SDA  [ 3] [ 4]  5V          (unused)
       I2C0 SCL  [ 5] [ 6]  GND ---- PN532 #1 GND
                       |
          GPIO4  [ 7] [ 8]  GPIO14 (TXD)      (both unused)
             GND  [ 9] [10]  GPIO15 (RXD)
                    |                (unused)
    PN532 #2 GND ---+
   Confirm button  GPIO17 [11] [12]  GPIO18  ---- WS2812 DIN (via resistor)
                  GPIO21 [13] [14]  GND  ---- LED strip GND tie-point
                  GPIO22 [15] [16]  GPIO23           (unused)
                    3.3V [17] [18]  GPIO24           (unused)
   SPI0 MOSI      GPIO10 [19] [20]  GND
   SPI0 MISO       GPIO9 [21] [22]  GPIO25          (unused)
   SPI0 SCLK      GPIO11 [23] [24]  GPIO8  (CE0) ---- PN532 #1 (tens) CS
                       GND [25] [26]  GPIO7  (CE1) ---- PN532 #2 (ones) CS
```

## PN532 readers (dual, shared SPI0 bus)

Both PN532 breakout boards share the same SCK/MOSI/MISO lines (pins 19,
21, 23) — that's normal for SPI with multiple devices. Only the chip-select
line is unique per board: reader 1 (the **tens** pad) uses CE0 (pin 24),
reader 2 (the **ones** pad) uses CE1 (pin 26).

- Set each PN532 module's mode jumper/switch to **SPI** (not I2C or HSU/UART
  — check your specific board's silkscreen/datasheet for which switch
  positions mean SPI, this varies by vendor).
- Power each from a 3.3V pin (1 and 17) — verify your specific breakout's
  supply voltage requirement first; the PN532 chip itself is 3.3V logic, but
  some breakout boards add a regulator that expects 5V input instead.
- No IRQ line wiring needed — polling mode (via `libnfc`) works without it.
- The recommended driver is `libnfc` against these two SPI devices, not a
  from-scratch PN532 protocol implementation (see Stage H2).

## WS2812 LED strip

- Data line: GPIO18 (pin 12) → a ~300-500Ω resistor → strip DIN. GPIO18 is
  the standard pin for this because it's the PWM0 peripheral pin that
  `rpi_ws281x` (the recommended driver, see Stage H3) targets.
- **Power the strip from a separate 5V supply, not the Pi's 5V pin.** A
  WS2812 strip of more than a handful of LEDs can draw well beyond what the
  Pi's own supply/polyfuse can safely provide — do this wrong and you risk
  browning out or damaging the Pi, not just the LEDs.
- Tie the external supply's ground to a Pi ground pin (e.g. pin 14) so both
  share a common reference — required for the data signal to work reliably,
  even though the Pi isn't powering the strip.
- A large capacitor (~1000µF) across the strip's + and - near its input is
  standard practice and worth including.
- A logic-level shifter (e.g. a 74AHCT125 buffer) between GPIO18's 3.3V
  signal and the strip's 5V-expecting data line is the "do it right" option;
  many small/short-wire setups work fine without one, so this is worth
  bench-testing before deciding you need the extra part.

## Confirm button

- One leg to GPIO17 (pin 11), the other to any GND pin (e.g. 6, 9, 14, 20,
  or 25).
- No external pull-up resistor needed — enable the Broadcom GPIO's internal
  pull-up in software (Stage H1) and treat the button as active-low (reads
  0 when pressed).
- No debounce circuit needed either if wired this way — debounce in
  software (Stage H1) instead, which is simpler to get right and to tune.

## Ergonomics reminder (from IMPLEMENTATION_PLAN.md Stage H4)

When you build the enclosure: aim for a large Confirm button face (roughly
2cm×2cm minimum) with a light-actuation-force switch rather than a standard
stiff arcade-button spring, keep the NFC pad recess shallow with no metal
hardware near the antenna area (test read range through your actual MDF
thickness before finalizing), and size the numeral tags/tiles generously
given the age-4 floor.
