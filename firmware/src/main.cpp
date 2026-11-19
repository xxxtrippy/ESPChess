// ESP32-S3 entry point. Wires the tested game core to the hardware HAL and adds
// a serial menu that selects the 24 configurations (8 game types x 3 opponents).
// The chess/game logic here is the SAME code proven by test/tests.cpp.
#include <Arduino.h>
#include "config.h"
#include "hal_hw.h"
#include "menu.h"
#include "board_map.h"

using namespace game;

static Session sess;
static uint32_t lastScan=0, lastRender=0;

static void announce(){
    Serial.printf("Selected: %s | %s | %s\n",
        gameTypeName(sess.gameType), oppName(sess.opp),
        sess.opp==OPP_AI ? (std::string("L")+std::to_string(sess.aiLevel)).c_str()
        : sess.opp==OPP_CHAMPIONSHIP ? "ladder@L1" : "-");
}

// Route a physical (row,col) press to the right handler.
static void routePress(int r,int c){
    long now=millis();
    int seg; hw::Control ctl=hw::controlAt(r,c,seg);
    if(ctl==hw::CTRL_WCLK)      sess.ctrl.onClock(chess::WHITE, now);
    else if(ctl==hw::CTRL_BCLK) sess.ctrl.onClock(chess::BLACK, now);
    else                        sess.ctrl.onCell(r,c,now);   // handles M-reset + squares
}

// Serial menu: g=game type, o=opponent, l=AI level, c=colour, n=start, p=fen.
static void handleSerial(){
    while(Serial.available()){
        switch(Serial.read()){
            case 'g': sess.menuNextType(); announce(); break;
            case 'o': sess.menuNextOpp();  announce(); break;
            case 'l': sess.menuLevelUp();  announce(); break;
            case 'c': sess.humanColor^=1;
                      Serial.printf("human plays %s\n", sess.humanColor==chess::WHITE?"white":"black"); break;
            case 'n': sess.start(millis()); announce();
                      Serial.printf("GAME START (%s)\n", sess.ctrl.board.toFEN().c_str()); break;
            case 'p': Serial.println(sess.ctrl.board.toFEN().c_str()); break;
            default: break;
        }
    }
}

void setup(){
    Serial.begin(115200);
    halhw::begin();
    sess.gameType=GT_STANDARD; sess.opp=OPP_HUMAN; sess.aiLevel=DEFAULT_AI_LEVEL;
    sess.start(millis());
    Serial.println("Chessboard ready. Menu: g=game o=opponent l=level c=colour n=start p=fen");
    announce();
}

void loop(){
    uint32_t now=millis();
    handleSerial();

    if(now-lastScan >= SCAN_INTERVAL_MS){
        lastScan=now;
        halhw::scan();
        int r,c;
        while(halhw::getMatrix().nextPress(r,c)) routePress(r,c);
    }

    bool wasOver = sess.ctrl.gameOver();
    sess.ctrl.update(now);                 // clock tick + AI move on its turn

    if(!wasOver && sess.ctrl.gameOver()){
        Serial.printf("Game over: result=%d\n", sess.ctrl.result);
        if(sess.opp==OPP_CHAMPIONSHIP){
            bool more = sess.onGameEnd(now);
            if(more) Serial.printf("Championship: next match at level %d\n", sess.champ.level);
            else if(sess.champ.finished) Serial.println("Championship: LADDER COMPLETE!");
            else Serial.println("Championship: run restarted (press n for a new ladder)");
        }
    }

    if(now-lastRender >= RENDER_INTERVAL_MS){
        lastRender=now;
        sess.ctrl.render(halhw::getLeds());
    }
}
