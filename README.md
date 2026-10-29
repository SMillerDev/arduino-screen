# RGB LED matrix on Arduino UNO R4 Minima

Driving a HUB75 RGB LED matrix from an Arduino UNO R4 Minima using
Adafruit's [Protomatter](https://github.com/adafruit/Adafruit_Protomatter)
library, with a local port of Protomatter to the Minima's Renesas RA4M1
chip (upstream doesn't support it).

## Hardware

- Arduino UNO R4 Minima
- Adafruit RGB Matrix Shield
- P4-32x32-16S HUB75 panel (32×32, 1/16 scan, no E pin)

The panel needs its own 5 V supply; don't power it from the Minima.

### Pins

The shield fixes the wiring:

| Signal | Pin |
|---|---|
| R1, G1, B1, R2, G2, B2 | D2, D3, D4, D5, D6, D7 |
| A, B, C, D | A0, A1, A2, A3 |
| CLK | D8 |
| OE | D9 |
| LAT | D10 |

In Protomatter's constructor the width is **32**, not 64.

## Sketches

| Sketch | Purpose |
|---|---|
| `simple/` | Adafruit's Protomatter "simple" example with Minima pins. Draws shapes, text and colour ramps (laid out for 64×32, so it's cramped on 32×32). Prints the begin status and refresh FPS every second. |

## Building and uploading

With `arduino-cli`, using the `Makefile`:

```
make simple-build    # compile + upload simple
make attach          # serial monitor at 9600 baud
```

`make attach` uses `/dev/cu.usbmodem3101`; check the port with
`arduino-cli board list` if it changes. Close the monitor before uploading,
as it holds the port.

### Serial monitor tips

- Pressing RESET disconnects the Minima's native USB, so the monitor stops
  and the start-up line is lost. The sketches repeat their status every
  second, so just open the monitor without resetting.
- `simple` prints `status 0, Refresh FPS = ~N`. A status of 0 means
  `begin()` succeeded; N above 0 means the display interrupt is running.

### Board disappeared from USB

Double-press RESET. The Minima enters the bootloader (LED pulses slowly)
and its port shows up again; upload as normal.

## The Protomatter RA4M1 port

Local changes in `libraries/Adafruit_Protomatter/`:

| File | Change |
|---|---|
| `src/arch/renesas.h` | New. GPIO, timer and data-output code for Renesas RA. |
| `src/arch/renesas.cpp` | New. Allocates a GPT timer channel through the core's `FspTimer` class and calls the row handler from its overflow interrupt. |
| `src/arch/arch.h` | Includes `renesas.h`. |
| `library.properties` | Adds `renesas_uno` to `architectures`. |

**Updating or reinstalling Protomatter from the Library Manager removes
these changes.** Keep a copy of the two `renesas.*` files.

### How it works

- **Timer:** a GPT channel clocked at PCLKD / 4 (12 MHz on the Minima), so
  even 16-bit channels can hold the longest bitplane period. The channel is
  allocated with `FspTimer` to avoid clashing with PWM; start, stop and
  count go straight to the registers because they're called from the ISR.
  If no free GPT channel is left, it takes one reserved for PWM, and
  `analogWrite()` stops working on that pin.
- **Clock on another port:** Protomatter normally requires RGB and CLK on
  the same GPIO port. On the shield, D2–D7 are PORT1 but D8 is PORT3. The
  port declares one byte per pixel element (`_PM_bytesPerElement 1`), which
  skips that check, and supplies its own `blast_byte()` that drives the
  clock through its own port. The RGB pins must still sit in one byte of
  one port, as D2–D7 do (P102–P107).
- **Pin writes:** every pin change is a single 32-bit write to the port's
  `PCNTR3` register: bits to set in the low half, bits to clear in the high
  half (the same encoding the core's SoftwareSerial uses). RGB data is
  written this way in one access per column, so each line only changes
  when its value does.

### RA4M1 gotchas found along the way

- **16-bit port register access is not what the header suggests.** The
  device header's 32-bit bitfields say `POSR` (set) is the low half of
  `PCNTR3` and `PODR` (output data) the high half of `PCNTR1`, but 16-bit
  accesses to those halves hit the *other* register (e.g. a 16-bit write at
  `PCNTR1 + 2` changes pin direction, not output). Using only 32-bit
  `PCNTR3` writes avoids the problem. Getting this wrong showed up as
  ghost rows, rows in the wrong place and a blank panel.
- **Keep the ISR short.** The display interrupt runs at top priority, and
  the Minima's USB is serviced at a lower one. A ~1 µs delay per clock edge
  starved USB completely and the board dropped off the bus. The clock hold
  is now about 200 ns (10 NOPs per edge).
- **`delayMicroseconds()` is a busy-wait** on this core
  (`R_BSP_SoftwareDelay`), so it's safe inside the ISR, just slow.

## Ideas

- Rework `simple`'s drawing for a 32×32 panel.
- Offer the RA4M1 port upstream to Adafruit as a pull request.
