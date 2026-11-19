// ESP32-S3 hardware implementations of the HAL (matrix scan + NeoPixel strip).
// Compiled on-device by PlatformIO (Arduino framework). The portable game core
// it feeds is validated by the host test suite.
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "hal.h"
#include "board_map.h"

namespace halimpl {

// ---------------- LED strip ----------------
class NeoLeds : public hal::ILeds {
    Adafruit_NeoPixel strip;
public:
    NeoLeds() : strip(NUM_LEDS, pins::LED_DATA, NEO_GRB + NEO_KHZ800) {}
    void begin(){ strip.begin(); strip.setBrightness(LED_BRIGHTNESS); strip.clear(); strip.show(); }
    void set(int i, hw::RGB c) override {
        if(i>=0 && i<NUM_LEDS) strip.setPixelColor(i, strip.Color(c.r, c.g, c.b));
    }
    void clear() override { strip.clear(); }
    void show() override { strip.show(); }
    int count() const override { return NUM_LEDS; }
};

// ---------------- matrix scanner with debounce + edge detection ----------------
class Matrix : public hal::IMatrix {
    bool stable[MATRIX_ROWS][MATRIX_COLS] = {};
    bool raw[MATRIX_ROWS][MATRIX_COLS] = {};
    uint32_t changedAt[MATRIX_ROWS][MATRIX_COLS] = {};
    // ring buffer of fresh presses
    struct Ev{uint8_t r,c;}; Ev q[32]; int qh=0, qt=0;
    void enqueue(int r,int c){ int n=(qt+1)&31; if(n!=qh){ q[qt]={(uint8_t)r,(uint8_t)c}; qt=n; } }
public:
    void begin(){
        for(int r=0;r<MATRIX_ROWS;r++){ pinMode(pins::ROW[r], OUTPUT); digitalWrite(pins::ROW[r], HIGH); }
        for(int c=0;c<MATRIX_COLS;c++){ pinMode(pins::COL[c], INPUT_PULLUP); }
    }
    // Call every SCAN_INTERVAL_MS.
    void scan(){
        uint32_t now = millis();
        for(int r=0;r<MATRIX_ROWS;r++){
            digitalWrite(pins::ROW[r], LOW);
            delayMicroseconds(5);
            for(int c=0;c<MATRIX_COLS;c++){
                bool pressed = (digitalRead(pins::COL[c]) == LOW); // active-low
                if(pressed != raw[r][c]){ raw[r][c]=pressed; changedAt[r][c]=now; }
                else if(now - changedAt[r][c] >= DEBOUNCE_MS && stable[r][c]!=pressed){
                    stable[r][c]=pressed;
                    if(pressed) enqueue(r,c);     // rising edge only
                }
            }
            digitalWrite(pins::ROW[r], HIGH);
        }
    }
    bool nextPress(int& r,int& c) override {
        if(qh==qt) return false;
        r=q[qh].r; c=q[qh].c; qh=(qh+1)&31; return true;
    }
};

static NeoLeds leds;
static Matrix  matrix;

} // namespace halimpl

// Accessors used by main.cpp
#include "hal_hw.h"
namespace halhw {
    void begin(){ halimpl::leds.begin(); halimpl::matrix.begin(); }
    void scan(){ halimpl::matrix.scan(); }
    hal::ILeds&   getLeds()   { return halimpl::leds; }
    hal::IMatrix& getMatrix() { return halimpl::matrix; }
}
