// Minimal Arduino API stub — ONLY for host compile-checking the ESP32 glue.
// Never used on-device (PlatformIO supplies the real Arduino core).
#pragma once
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <string>

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2

inline unsigned long millis(){ return 0; }
inline unsigned long micros(){ return 0; }
inline void pinMode(int,int){}
inline void digitalWrite(int,int){}
inline int  digitalRead(int){ return HIGH; }
inline void delay(unsigned long){}
inline void delayMicroseconds(unsigned int){}

struct SerialStub {
    void begin(long){}
    int  available(){ return 0; }
    int  read(){ return -1; }
    void println(const char*){}
    void println(){}
    void print(const char*){}
    int  printf(const char* f, ...){ (void)f; return 0; }
};
static SerialStub Serial;
