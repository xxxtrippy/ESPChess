// Board/hardware configuration. Keep ALL pin + geometry assumptions here so the
// unverified GPIO map (from the project handoff) is changed in one place.
#pragma once

// ---------------- matrix geometry ----------------
#define MATRIX_ROWS 9
#define MATRIX_COLS 9
#define NUM_LEDS    81

// ---------------- GPIO map (PROVISIONAL — verify against your board!) ----------
// From the handoff; NOT electrically verified. Check boot/strapping pins.
namespace pins {
    // 9 row drive lines, 9 column sense lines
    static const int ROW[MATRIX_ROWS] = {1, 2, 4, 5, 6, 7, 8, 9, 10};
    static const int COL[MATRIX_COLS] = {11,12,13,14,15,16,17,18,21};
    static const int LED_DATA = 47;
}

// Scan polarity: rows driven LOW one at a time, columns read with pull-ups.
// A pressed key reads LOW on its column. Diode cathode -> ROW (see wiring).
#define MATRIX_ACTIVE_LOW   1
#define DEBOUNCE_MS         8
#define SCAN_INTERVAL_MS    2
#define RENDER_INTERVAL_MS  33     // ~30 fps LED refresh

// ---------------- LED ----------------
#define LED_BRIGHTNESS      90     // global cap (0-255) — keep current in check!
#define LED_MAX_MILLIAMPS   4000   // soft current limit for the strip

// ---------------- defaults at power-on ----------------
#define DEFAULT_MODE        game::STANDARD
#define DEFAULT_OPPONENT    game::HUMAN_VS_HUMAN
#define DEFAULT_AI_LEVEL    5
