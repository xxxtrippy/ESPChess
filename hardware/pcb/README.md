# 9×9 Chessboard PCB — KP Republic 182×182 mm plate

Two‑board KiCad 8 design, all **through‑hole** (hand‑solderable, no SMD):

1. **PLATE board** `ChessboardPCB` — 182×182 mm, mounts the 81 MX switches of the
   KP Republic 9×9 switch‑tester plate, with one 5 mm addressable RGB LED and one
   1N4148 matrix diode per switch. Connects to the control board by a ribbon (J1).
2. **CONTROL board** `ChessboardCtrl` — 70×55 mm, holds the ESP32 breakout,
   74AHCT125 level shifter, data resistor, bulk cap and 5 V input. Connects to the
   plate board by the matching ribbon (J2).

The switch grid fills the plate (152.4 mm span in 182 mm → ~14.8 mm margins), so
the control electronics live on their own small board instead of being crammed
into the edge.

## Files

- `ChessboardPCB.kicad_pcb` / `.kicad_pro` — plate board (open in KiCad 8).
- `ChessboardCtrl.kicad_pcb` / `.kicad_pro` — control board.
- `plate_render.png`, `ctrl_render.png` — top‑view renders.
- `matrix_map.csv` — per switch: physical row/col, ROW/COL net, LED chain index,
  chess square (A1–H8) or control role (M, E1–E7, S1–S7, clocks).
- `BOM.csv` — parts for both boards.

**State:** both boards are fully **placed and netlisted** (full ratsnest in
KiCad). **Tracks are not routed yet** — see “Finish the boards”. Footprints are
custom and embedded (library names `Chessboard:*` / `Ctrl:*`), so the projects
open with no external libraries; verify the custom ones before a full build.

## Board orientation (matches the firmware handoff — authoritative)

White at the bottom, Black at the top, never rotated.

- Physical `row 0` = **top** (Black). `row 8` = **bottom** (White).
- `(0,0)` = **M**. `(0,8)` = **BLACK CLK**. `(8,0)` = **WHITE CLK**.
- `(0,1..7)` = E1–E7. `(1..7,0)` = S1–S7.
- Chess squares `(1..8,1..8)`: `A1 = (8,1)` bottom‑left, `H8 = (1,8)` top‑right.
- `SW{n}` and `D{n}` are numbered `row*9 + col + 1`.

Plate centred on the board: 19.05 mm pitch, M3 holes in the corners.

## Matrix wiring (plate board)

9 rows × 9 cols, one 1N4148 per key:

```
COL(c) ── switch ── SWD_r_c ── 1N4148(anode→cathode) ── ROW(r)
```

Diode pad 1 is the banded (cathode) end → `ROW(r)`. **Diode direction and
which lines you drive vs. read must match your `config.h` scan code** (the
handoff flags this). Keep the pin map configurable, not hard‑coded.

## LED chain (plate board)

81 × 5 mm addressable LEDs (WS2811 driver inside, WS2812‑compatible). They are
daisy‑chained in the exact serpentine your firmware already uses, so
`ledIndex()` is guaranteed correct by the layout:

```cpp
uint16_t ledIndex(uint8_t row, uint8_t col) {
    if ((row & 1) == 0) return row * 9 + col;   // even rows L→R
    return row * 9 + (8 - col);                 // odd rows R→L
}
```

`LED1` (index 0) is at `(0,0)`; the data signal snakes row by row to index 80.
Data arrives from the control board as `DATA_LED` (already level‑shifted and
through the series resistor).

## The ribbon between the boards (J1 ⇄ J2, 2×13, identical order)

| Pin | Net | Pin | Net |
|----|-------|----|-------|
| 1  | ROW0  | 14 | COL0 |
| 2  | ROW1  | 15 | COL1 |
| 3  | ROW2  | 16 | COL2 |
| 4  | ROW3  | 17 | COL3 |
| 5  | ROW4  | 18 | COL4 |
| 6  | ROW5  | 19 | COL5 |
| 7  | ROW6  | 20 | COL6 |
| 8  | ROW7  | 21 | COL7 |
| 9  | ROW8  | 22 | COL8 |
| 10 | +5V   | 23 | +5V  |
| 11 | GND   | 24 | GND  |
| 12 | DATA_LED | 25 | +5V |
| 13 | DOUT_END | 26 | GND |

Use a 26‑way IDC ribbon (or pin‑to‑pin). `DOUT_END` is the last LED’s DOUT,
brought back for diagnostics — leave unconnected if unused. Power pins are
trebled (3× +5V, 3× GND) so the ribbon can carry LED current; for full
brightness consider feeding 5 V to the plate directly as well.

## Control board (ChessboardCtrl)

- **J3** — 2×11 ESP32‑S3 breakout. Flylead/dupont to the dev board. Left column:
  ROW0–8, GND, +5V. Right column: COL0–8, MCU_DATA, GND. Wire to whatever GPIOs
  you set in `config.h`; the handoff’s provisional pins (rows 1–10, cols 11–21,
  data 47) are **unverified** — confirm against your module’s strapping/boot pins.
- **U1** 74AHCT125 (DIP‑14) — shifts the ESP32 3.3 V data to 5 V. Buffer 1 used:
  1OE→GND (enabled), 1A←MCU_DATA, 1Y→DATA_SHIFTED; unused buffers disabled.
- **R1** 470 Ω — series on the data line (DATA_SHIFTED → DATA_LED).
- **C1** 1000 µF — bulk cap across 5 V / GND.
- **J4** — 5 V input (from your external supply).
- **J2** — ribbon to the plate (pinout above).

## Power (read before building)

- 81 LEDs at full white draw **~50 mA each ≈ 4 A @ 5 V worst case.** **Do not
  power the LEDs from the ESP32 3.3 V rail or USB alone.**
- Feed regulated **5 V** into **J4** on the control board; share a **common
  ground** with the ESP32.
- Cap brightness/current in firmware so you never actually reach 4 A. For full
  brightness, also inject 5 V/GND straight onto the plate’s +5V/GND, don’t rely
  on the ribbon alone.
- When routing, give +5V/GND wide tracks or copper pours.

## Finish the boards in KiCad (routing)

1. Open each `.kicad_pro` in **KiCad 8** → PCB editor. Everything is placed; the
   white ratsnest shows all connections.
2. Plate board — fastest clean matrix route: **ROWs** horizontal on **B.Cu**,
   **COLs** vertical on **F.Cu** (they cross only on opposite layers); add **GND
   and +5V copper pours** on both layers; route the short LED `CHAIN*` data hops
   last. Or export **Specctra DSN** and autoroute with Freerouting.
3. Control board — a handful of parts, route by hand in minutes.
4. Run **DRC** (clearance 0.2 mm / track 0.3 mm) on both, fix, then Gerbers.

## Verify before building (custom footprints)

1. **Addressable LED pinout — most important.** Footprint pads, left→right, are
   **`+5V, DIN, GND, DOUT`** (pin‑1 `+5V` marked on silk). Vendor pinouts differ —
   check your LED’s datasheet and swap pad/net assignment if needed. The 4 leads
   form to a 2.54 mm inline pitch for hand‑soldering.
2. **MX switch** — standard Cherry pin positions, 4 mm centre hole, two 1.75 mm
   leg holes; plate‑mount (3‑pin) and 5‑pin PCB‑mount both drop in. Fit‑test one
   against your plate first.
3. **1N4148** — DO‑35, 7.62 mm pitch; band (cathode) = pad 1 = ROW side.
4. Build and test **one full cell + a 3‑LED chain** before populating all 81.

## Enclosure — 3D-printed sandwich (top plate · PCB · bottom plate)

`top_plate.stl`, `bottom_plate.stl` (previews: `plates_top_view.png`,
`sandwich_section.png`). Both are 182×182 mm and share the PCB's 4 corner M3
holes.

Stack, top to bottom:

```
TOP PLATE (3mm, 81 MX cutouts)
   └─ 3.5mm gap  ← set by the SWITCH itself (corner legs are printed spacers)
PCB (1.6mm)
   └─ 6mm M3 standoff ×4  ← clears bottom-side pins / solder / LED legs
BOTTOM PLATE (3mm, flat)
```

**Key constraint — the top gap is not free.** An MX switch fixes the
plate‑to‑PCB distance at ~3.4–3.5 mm, so you **cannot** put a tall motherboard
standoff between the top plate and the PCB — the pins wouldn't reach. The top
plate therefore has four **3.5 mm corner legs** printed on its underside that
rest on the PCB and keep the plate parallel; the switches carry the rest. The
**M3 standoffs go between the PCB and the bottom plate**, where the height is
free (6 mm clears the trimmed pins and solder).

Top-plate switch holes are **stepped**: a 1.5 mm latch web (14 mm opening) on
top so the MX clips actually snap in, opening to 15.6 mm relief below so the
clips can spring. Print the top plate **web-side up** (holes get a 0.8 mm
down-facing ledge that bridges fine, no supports).

Fasteners per corner:
- **M3×12** from the top: through top plate + corner leg + PCB, into the
  standoff's top thread.
- **M3×6** from the bottom: through the bottom plate into the standoff's bottom
  thread.
- 4× **M3 6 mm female-female** standoffs.

Print notes: PETG or PLA, ~0.2 mm layers, 3+ walls. Check one switch fits a test
cutout before printing the whole plate — tune `MX_CUT` (14.0) and the 3.5 mm leg
height in `scripts/gen_plates.py` to your switches/printer, then re-run. The
bottom plate has a 24×10 mm cable slot centered on one edge for the ESP32 lead.
