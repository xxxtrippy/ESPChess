// Minimal Adafruit_NeoPixel stub for host compile-checking only.
#pragma once
#include <cstdint>
#define NEO_GRB 0x01
#define NEO_KHZ800 0x02
class Adafruit_NeoPixel {
public:
    Adafruit_NeoPixel(uint16_t,uint16_t,uint16_t){}
    void begin(){}
    void setBrightness(uint8_t){}
    void clear(){}
    void show(){}
    void setPixelColor(uint16_t,uint32_t){}
    uint32_t Color(uint8_t r,uint8_t g,uint8_t b){ return (r<<16)|(g<<8)|b; }
};
