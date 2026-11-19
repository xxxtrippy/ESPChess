// ESP32 hardware accessors (implemented in hal_esp32.cpp).
#pragma once
#include "hal.h"
namespace halhw {
    void begin();
    void scan();
    hal::ILeds&   getLeds();
    hal::IMatrix& getMatrix();
}
