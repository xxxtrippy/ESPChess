// Chess clocks with increment. Header-only; pure logic, time injected as ms.
#pragma once
#include <cstdint>
#include "chess.h"

namespace game {

struct TimeControl { const char* name; long baseMs; long incMs; bool untimed; };

namespace tc {
    inline TimeControl Standard(){ return {"Standard",0,0,true}; }
    inline TimeControl Rapid()   { return {"Rapid",15*60000,10000,false}; } // 15|10
    inline TimeControl Blitz()   { return {"Blitz",5*60000,0,false}; }      // 5|0
    inline TimeControl Bullet()  { return {"Bullet",1*60000,0,false}; }     // 1|0
}

class Clock {
public:
    TimeControl tc{"Standard",0,0,true};
    long remaining[2]={0,0};
    int active=chess::WHITE;   // whose clock is ticking
    bool running=false;
    int flaggedPlayer=-1;      // -1 none
    long lastTick=0;

    void configure(const TimeControl& t, long nowMs){
        tc=t; remaining[0]=remaining[1]=t.baseMs; active=chess::WHITE;
        running=!t.untimed; flaggedPlayer=-1; lastTick=nowMs;
    }
    // call frequently; decrements the active player's time
    void update(long nowMs){
        if(tc.untimed||!running||flaggedPlayer>=0){ lastTick=nowMs; return; }
        long dt=nowMs-lastTick; lastTick=nowMs;
        if(dt<0) dt=0;
        remaining[active]-=dt;
        if(remaining[active]<=0){ remaining[active]=0; flaggedPlayer=active; running=false; }
    }
    // The active player finished a legal move and pressed THEIR clock.
    // Returns true if it was the correct (active) player's clock.
    bool press(int player, long nowMs){
        update(nowMs);
        if(flaggedPlayer>=0) return false;
        if(player!=active) return false;           // wrong clock: nothing happens
        if(!tc.untimed) remaining[active]+=tc.incMs; // award increment
        active^=1;                                  // switch
        lastTick=nowMs;
        return true;
    }
    bool expired() const { return flaggedPlayer>=0; }
    int flagged() const { return flaggedPlayer; }
    long ms(int player) const { return remaining[player]; }
};

} // namespace game
