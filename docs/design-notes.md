# Chessboard Project Handoff

**Last updated:** 2026-11-19  
**Purpose:** Handoff document for continuing the custom 9×9 physical chessboard / ESP32-S3 firmware project.

## 1. Project overview

Build a custom electronic chessboard using:

- ESP32-S3 development board, advertised as **N16R8 / N16R18** (the user describes the common AliExpress dual-USB-C board).
- 81 mechanical switches in a 9×9 physical layout.
- One addressable RGB LED per switch (81 total, WS2812B-compatible assumed).
- A chess engine supporting human play, AI play, clocks, championships, puzzles and chess variants.

The user expects a real PlatformIO project and working firmware, not merely a conceptual architecture. Do not claim a ZIP or firmware files exist unless they have actually been created.

## 2. Physical board layout — authoritative

The board is always viewed with **White at the bottom** and **Black (Player 2 or AI) at the top**. It is never rotated.

The 9×9 matrix contains 17 control/evaluation positions and 64 chess squares:

- `(0,0)`: `M` multifunction mode/menu/reset button.
- `(0,1)` through `(0,7)`: `E1` through `E7`, top evaluation-bar segments.
- `(0,8)`: **Black CLK**, top-right corner.
- `(1,0)` through `(7,0)`: `S1` through `S7`, left-side evaluation-bar segments.
- `(8,0)`: **White CLK**, bottom-left corner.
- `(1..8, 1..8)`: the 64 chess squares.

There are exactly **two clock buttons**:
- White clock: bottom-left `(8,0)`.
- Black clock: top-right `(0,8)`.

Do not mistakenly label the whole first column as clock buttons. `S1–S7` are evaluation segments.

### Chess coordinate orientation

Files A–H run left-to-right from White's perspective. Ranks 1–8 run bottom-to-top.

| Physical matrix coordinate | Chess square |
|---|---|
| `(8,1)` | A1 |
| `(8,8)` | H1 |
| `(1,1)` | A8 |
| `(1,8)` | H8 |

Conversion from physical matrix row/column to chess file/rank:

```cpp
int file = physicalCol - 1; // 0=A ... 7=H
int rank = 8 - physicalRow; // 0=rank 1 ... 7=rank 8
int square = rank * 8 + file; // 0=A1 ... 63=H8
```

Starting position:
- White back rank: R-N-B-Q-K-B-N-R on A1–H1; pawns A2–H2.
- Black back rank: R-N-B-Q-K-B-N-R on A8–H8; pawns A7–H7.

## 3. Controls and evaluation display

- E1–E7 and S1–S7 form the 14-segment evaluation display around the L-shaped edge.
- In timed games, these segments should automatically display remaining clock/time information instead of engine evaluation.
- The top-left `M` button is for mode/menu/reset functions.
- The two clock buttons are used to complete the current player's move:
  - White presses White CLK at bottom-left.
  - Black presses Black CLK at top-right.
  - Pressing the wrong player's clock does nothing.
  - On a valid move completion, award increment (if configured) and switch active player/clock.
  - Clock expiration ends the game.

The exact interaction model for selecting and moving pieces with chess-square switches still needs to be implemented and tested. Switches detect presses; they do not detect piece identity or physical piece presence.

## 4. LED behavior

- 81 addressable RGB LEDs, one per switch.
- Assumed LED protocol: WS2812B-compatible; verify actual LED type and wiring before finalizing.
- Proposed piece colors:
  - Pawn: green `(0,255,0)`
  - Knight: purple `(160,0,255)`
  - Rook: amber/orange `(255,140,0)`
  - Bishop: blue `(0,80,255)`
  - Queen: red `(255,0,0)`
  - King: yellow `(255,255,0)`
- Active player's pieces: 100% brightness.
- Waiting player's pieces: 50% brightness.
- Apply the brightness distinction in both human-vs-human and human-vs-AI games.
- Empty squares normally off. Selection, legal-move, check, checkmate and status indications may use additional highlights/animations.
- Ensure the LED renderer does not clear or overwrite auxiliary control/evaluation LED states unintentionally.

A tentative serpentine LED index mapping was proposed:

```cpp
uint16_t ledIndex(uint8_t row, uint8_t col) {
    if ((row & 1) == 0) return row * 9 + col;
    return row * 9 + (8 - col);
}
```

This is only correct if the physical LED strip actually snakes in that direction. Confirm the actual chain routing before relying on it.

## 5. Switch matrix and electrical design

- 9 row lines + 9 column lines = 18 GPIOs.
- One diode per switch to prevent ghosting.
- Proposed scan method: activate one row at a time and read all columns, with software debounce.
- Diode orientation and active polarity must be matched to the actual scan circuit and verified on a small test section before building all 81 switches.
- Mechanical switch matrix detects key presses only; it does not sense chess pieces.

### Provisional GPIO proposal

The following was suggested for the common dual-USB-C ESP32-S3 board, but **has not been electrically verified against the user's exact PCB revision**:

| Function | GPIO |
|---|---|
| Matrix rows R0–R8 | 1, 2, 4, 5, 6, 7, 8, 9, 10 |
| Matrix columns C0–C8 | 11, 12, 13, 14, 15, 16, 17, 18, 21 |
| WS2812 data | 47 |

Treat this as a draft, not a fabrication-ready pinout. Confirm pin labels and module/carrier revision before wiring. Check all pins for board-specific use, boot/strapping behavior and peripheral conflicts.

Avoid assuming every AliExpress board sold under the same listing has identical routing. The advertised `N16R8` designation normally means 16 MB flash and 8 MB PSRAM; a seller's `N16R18` wording may be a listing typo or variant. Confirm the module marking and PSRAM mode/size.

### LED power

- Do not power 81 LEDs from the ESP32 3.3 V rail.
- Use a suitably rated regulated 5 V supply for LEDs.
- Use a common ground between ESP32 and LED supply.
- Recommended signal conditioning: 74AHCT125 level shifter and a series data resistor (330 Ω is a common starting value).
- A bulk capacitor near LED power entry (e.g. 1000 µF, with suitable voltage rating) is a common stability measure.
- Inject 5 V and ground at multiple points as required by measured current and voltage drop.
- Add appropriate fuse/protection and wire gauge.
- Do not connect external 5 V and USB 5 V together without a correctly designed power-path arrangement.
- Full-white worst-case current can be substantial; set a brightness/current limit and size the supply and wiring from the actual LED model's datasheet and measured load.

## 6. Game modes and rules requested

Boot default: standard human-vs-human chess.

Game families requested:
1. Standard chess
2. Puzzle mode with randomized starting positions
3. Fog of War
4. Horde
5. King of the Hill

Opponent configurations requested:
- Human vs human
- Human vs selected ELO/difficulty AI
- AI championship

The user described this as 8 game families × 3 opponent modes / 24 main configurations. The exact full list of the eight families has not been fully enumerated in the preserved context; do not invent the missing families. Ask the user to confirm the remaining families when implementing the final menu.

Time controls:
- Standard: untimed.
- Rapid: 15 minutes + 10 seconds increment.
- Blitz: 5 minutes, no increment.
- Bullet: 1 minute, no increment.

Fog of War rule specified by user: the full board remains visible, but enemy piece identities are hidden.

Horde and King of the Hill need their correct variant-specific starting positions, legality and win conditions implemented. Puzzle mode should generate randomized positions; puzzle opponent/solving semantics need confirmation if they affect implementation.

### AI strength

- Ten selectable strength levels requested, from extremely weak/easy to extremely strong.
- User described endpoints conceptually as “newborn baby—impossible to lose” and “9999 ELO—impossible to win.”
- Do not claim a real 9999 Elo rating. Embedded engine strength depends on engine, hardware, time controls and benchmarking. Implement ten strength profiles and describe them as levels unless calibrated with a recognized rating test.
- Championship: win advances to next level; draw replays the same match; loss restarts at level 1.

## 7. Suggested firmware structure

PlatformIO project, likely Arduino framework initially unless there is a strong reason to use ESP-IDF:

```text
Chessboard/
├── platformio.ini
├── include/
│   ├── config.h
│   ├── board.h
│   ├── chess.h
│   ├── engine.h
│   └── modes.h
└── src/
    ├── main.cpp
    ├── matrix.cpp
    ├── leds.cpp
    ├── chess.cpp
    ├── engine.cpp
    ├── clock.cpp
    ├── evaluation.cpp
    ├── puzzles.cpp
    └── modes.cpp
```

Suggested implementation order:
1. Hardware bring-up: scan all 81 switches, debounce, report physical row/column.
2. LED bring-up: address all 81 LEDs, verify physical index and control positions.
3. Coordinate conversion and board orientation tests.
4. Chess position representation and legal move generation.
5. Check, checkmate, stalemate, castling, en passant and promotion.
6. Human-vs-human input flow and clocks.
7. AI engine integration and ten strength profiles.
8. Championship flow.
9. Puzzle, Fog of War, Horde and King of the Hill.
10. Evaluation/timer display, polish and persistence.

A hardware test scaffold is not a finished chess engine. Keep those milestones distinct.

## 8. Current status / what has and has not happened

Established:
- The user confirmed their board looks like the common AliExpress dual-USB-C ESP32-S3 N16R8-style development board shown in the reference image.
- Physical 9×9 layout and clock-button locations are corrected and authoritative.
- Chess coordinate orientation is defined.
- Core LED colors, brightness behavior, game/time-control requests and broad firmware architecture are documented.

Not yet completed:
- No complete PlatformIO source project has been delivered.
- No downloadable ZIP has been created.
- No final pinout has been verified against a clear photo or exact PCB revision.
- No complete chess engine, AI, variant implementation or hardware test has been delivered.
- No actual switch/LED wiring has been confirmed.

## 9. Immediate next steps

1. Start by creating a real PlatformIO project with compileable hardware bring-up firmware.
2. Keep the pin map configurable in `config.h`; do not hard-wire unverified GPIO assumptions throughout the code.
3. Implement the matrix and LED tests first, with a serial diagnostic mode.
4. Validate all physical positions, especially:
   - M top-left
   - Black CLK top-right
   - White CLK bottom-left
   - E1–E7 across top
   - S1–S7 down left
   - A1 bottom-left chess square
   - H8 top-right chess square
5. Only after hardware validation, implement chess rules and the engine.

**Critical recurring correction:** White is always at the bottom. White CLK is bottom-left. Black is always at the top. Black CLK is top-right. The first column below M contains S1–S7 evaluation segments, not clock buttons.
