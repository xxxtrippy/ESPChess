// Hardware abstraction: same game logic runs on ESP32 and on the host simulator.
#pragma once
#include "board_map.h"

namespace hal {

// Switch matrix: yields debounced "just pressed" events as (row,col).
struct IMatrix {
    virtual ~IMatrix(){}
    virtual bool nextPress(int& row, int& col) = 0;  // false if none pending
};

// LED strip (81 addressable).
struct ILeds {
    virtual ~ILeds(){}
    virtual void set(int index, hw::RGB c) = 0;
    virtual void clear() = 0;
    virtual void show() = 0;
    virtual int  count() const = 0;
};

// Monotonic milliseconds.
struct ITime {
    virtual ~ITime(){}
    virtual long now() = 0;
};

} // namespace hal
