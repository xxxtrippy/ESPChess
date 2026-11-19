# 9×9 Electronic Chessboard — ESP32-S3 firmware

PlatformIO firmware for the custom 81-switch + 81-addressable-LED chessboard.
The chess/game logic is written as a **portable core** (no Arduino dependencies)
so it can be exhaustively tested on a PC, and a thin ESP32 layer drives the real
hardware.

## What has been verified (host test suite)

Run `./run_host_tests.sh` (needs `g++`). Current result:

```
71 unit/integration tests PASS   (0 fail)  — incl. all 24 game configs
20/20 perft positions PASS       (startpos depth 5 = 4,865,609,
                                  kiwipete depth 4 = 4,085,603)
```

`test/demo_game.cpp` also plays full games end-to-end (see `test/` output):
- scripted human game → **Scholar's mate** detected (checkmate, 1-0)
- AI vs AI self-play to completion in **Standard**, **King of the Hill**
  (king reaches d4 → win) and **Horde**

What the tests prove correct:
- **Move generation / rules** — castling, en passant, promotion, check,
  checkmate, stalemate, 50-move & insufficient-material draws (perft-verified).
- **Board mapping** — `(row,col)↔square` (A1=(8,1), H8=(1,8)), serpentine
  `ledIndex` is a clean 0..80 permutation, piece colours + 100%/50% brightness.
- **Clocks** — Standard/Rapid 15|10/Blitz 5|0/Bullet 1|0, increment, wrong-clock
  no-op, flag on timeout.
- **Variants** — Fog of War (enemy identities hidden), Horde (36-pawn start,
  all-captured → loss), King of the Hill (centre-square win), Puzzle (random
  legal positions).
- **AI** — 10 strength levels; finds mate-in-1 at top level; strong level beats
  weak level in most games.
- **Full control path** — select → stage → clock-to-commit → result, plus
  championship ladder (win advances, draw replays, loss resets to 1).

## What is NOT (and cannot be) verified without the bench

This is firmware proven in **simulation**. It has **not** run on real silicon
here. Before trusting it on hardware, you must still:
- Confirm the **GPIO map** in `include/config.h` (handoff-provisional) against
  your exact board and its boot/strapping pins.
- Confirm the **addressable-LED pinout** and matrix **diode direction** vs your
  wiring (flip scan polarity / diode sense in `config.h` if needed).
- Confirm **power** (external 5 V, common ground, current budget).
- Flash it and bring up matrix + LEDs with the serial diagnostics.

## Layout

```
platformio.ini            ESP32-S3 build (Arduino + Adafruit NeoPixel)
include/  config.h        pins/geometry/options  (EDIT pins here)
          chess.h         portable engine API
          board_map.h     matrix<->square, ledIndex, colours
          clock.h clock logic   variants.h variants   engine.h AI
          game.h controller      championship.h ladder   hal.h / hal_hw.h
src/      chess.cpp variants.cpp engine.cpp game.cpp   (portable core)
          hal_esp32.cpp   NeoPixel + debounced matrix scanner
          main.cpp        setup()/loop(), input routing, serial menu
test/     tests.cpp       63-assert suite (perft+rules+variants+AI+play)
          perft_main.cpp  standalone deep perft
          demo_game.cpp   prints full play-throughs
          sim.h stubs/    host fakes + Arduino stubs (compile-check only)
run_host_tests.sh         build + run everything on your PC
```

## Build / flash (on your machine)

```
# host tests (no hardware):
./run_host_tests.sh

# device (PlatformIO installed):
pio run                 # compile for ESP32-S3
pio run -t upload       # flash
pio device monitor      # 115200 baud
```

Serial menu while running: `g` game type · `o` opponent · `l` AI level ·
`c` human colour · `n` start · `p` print FEN.

## Controls (on the board)

- **Chess squares** — tap your piece, then its destination (legal targets light
  green); press **your clock** to commit the move.
- **White CLK** (8,0) / **Black CLK** (0,8) — complete your move / switch turn.
- **M** (0,0) — reset the current game. (Mode/opponent selection is on serial
  now; an on-board M-menu using the E/S segments is the natural next UI step.)
- Promotion currently auto-queens.

## The 24 configurations (8 game types x 3 opponents)

Defined in `include/menu.h`, all 24 verified by the test suite.

| Game type | Mode | Time |
|---|---|---|
| Standard | standard | untimed |
| Rapid | standard | 15 \| 10 |
| Blitz | standard | 5 \| 0 |
| Bullet | standard | 1 \| 0 |
| Puzzle | random legal pos | untimed |
| Fog of War | enemy ID hidden | untimed |
| Horde | 36-pawn horde | untimed |
| King of the Hill | centre-win | untimed |

Opponent modes: **2 Players** · **Vs AI** (selectable level 1–10) ·
**Championship** (climb all 10 AI levels; win advances, draw rematches, loss
restarts at level 1). `Session` in `menu.h` owns the selection, the live game and
the championship ladder, and auto-starts the next match as the ladder progresses.

## Honest scope notes

- **AI levels are depth/blunder profiles, not calibrated Elo** — labelled L1–L10,
  not "9999", exactly as the handoff requires.
- Puzzle mode generates random **legal** positions; puzzle *solving* semantics
  (goal/scoring) can be layered on once you define them.
- Promotion auto-queens; human = white by default (`c` toggles colour). An
  on-board M-button menu (game type / opponent / level on the E/S segments) is
  the natural next UI step — the menu *model* already exists and is tested, only
  the physical button mapping remains.
