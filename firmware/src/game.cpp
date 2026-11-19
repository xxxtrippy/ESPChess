#include "game.h"
#include "engine.h"
#include <vector>
using namespace chess;

namespace game {

void Controller::newGame(const Config& c, long now){
    cfg=c; rng = c.seed? c.seed : 0xC0FFEE;
    setupPosition(board, c.mode, c.seed);
    clock.configure(c.tc, now);
    result=ONGOING; selFrom=-1; stagedFrom=stagedTo=-1; stagedPromo=Q;
    refreshResult();
}

bool Controller::legalTarget(int from,int to) const {
    std::vector<Move> l; const_cast<Board&>(board).genLegal(l);
    for(auto& m: l) if(m.from==from && m.to==to) return true;
    return false;
}

void Controller::refreshResult(){ result = game::result(board, cfg.mode); }

bool Controller::isAITurn() const {
    if(cfg.opponent==AI_VS_AI) return true;
    if(cfg.opponent==HUMAN_VS_AI) return board.side==cfg.aiColor;
    return false;
}

void Controller::onCell(int row, int col, long now){
    if(gameOver()) return;
    int seg; hw::Control ctl=hw::controlAt(row,col,seg);
    if(ctl==hw::CTRL_M){ newGame(cfg, now); return; }    // M = reset current game
    if(ctl!=hw::CTRL_NONE) return;                        // E/S segments aren't inputs
    int sq=hw::matrixToSquare(row,col);
    if(sq<0) return;
    if(isAITurn()) return;                                // ignore board presses on AI's turn

    int pc=board.sq[sq];
    if(selFrom<0){
        if(pc!=EMPTY && pcolor(pc)==board.side){ selFrom=sq; }  // select own piece
        return;
    }
    if(sq==selFrom){ selFrom=-1; stagedFrom=stagedTo=-1; return; } // deselect
    if(pc!=EMPTY && pcolor(pc)==board.side){ selFrom=sq; stagedFrom=stagedTo=-1; return; } // reselect
    if(legalTarget(selFrom,sq)){
        stagedFrom=selFrom; stagedTo=sq; stagedPromo=Q;   // stage move (auto-queen)
    }
}

bool Controller::commitStaged(long now){
    if(stagedFrom<0||stagedTo<0) return false;
    std::vector<Move> l; board.genLegal(l);
    for(auto& m: l){
        if(m.from==stagedFrom && m.to==stagedTo){
            if(m.promo && m.promo!=stagedPromo) continue;  // match promotion choice
            Undo u; board.makeMove(m,u);
            selFrom=-1; stagedFrom=stagedTo=-1;
            refreshResult();
            return true;
        }
    }
    return false;
}

void Controller::onClock(int player, long now){
    if(gameOver()) return;
    if(player!=board.side) return;            // wrong clock does nothing
    if(isAITurn()) return;                     // AI commits itself
    if(stagedFrom<0) return;                    // no completed move to confirm
    if(commitStaged(now)) clock.press(player, now);
}

void Controller::doAIMove(long now){
    Move m; int level = (cfg.opponent==AI_VS_AI)
        ? (board.side==WHITE?cfg.aiLevelWhite:cfg.aiLevelBlack) : cfg.aiLevel;
    if(!ai::pickMove(board, cfg.mode, level, rng, m)) return;
    int mover=board.side;
    Undo u; board.makeMove(m,u);
    refreshResult();
    clock.press(mover, now);                    // AI "presses" its clock
}

void Controller::update(long now){
    clock.update(now);
    if(gameOver()) return;
    if(isAITurn()) doAIMove(now);
}

// ---------------- LED rendering ----------------
void Controller::render(hal::ILeds& leds){
    leds.clear();
    int viewer = board.side;   // active player's viewpoint (for fog)
    // board squares
    for(int sq=0;sq<64;sq++){
        int pc=board.sq[sq];
        hw::RGB col{0,0,0};
        if(pc!=EMPTY){
            if(fogReveals(board,cfg.mode,viewer,sq))
                col = hw::pieceRender(pc, board.side);
            else
                col = {40,40,40};   // hidden enemy identity (fog): neutral grey
        }
        leds.set(hw::squareToLed(sq), col);
    }
    // selection + legal targets
    if(selFrom>=0){
        leds.set(hw::squareToLed(selFrom), {255,255,255});
        std::vector<Move> l; board.genLegal(l);
        for(auto& m: l) if(m.from==selFrom)
            leds.set(hw::squareToLed(m.to), {0,120,0});
    }
    if(stagedTo>=0) leds.set(hw::squareToLed(stagedTo), {0,200,0});
    // check indicator
    if(board.inCheck(board.side)){
        int ks=board.kingSquare(board.side);
        if(ks>=0) leds.set(hw::squareToLed(ks), {255,0,0});
    }
    // segment display (E1..E7 top, S1..S7 left)
    int litE=0, litS=0;
    if(cfg.tc.untimed){
        int ev = ai::evaluate(board, cfg.mode);     // stm perspective
        int whiteAdv = (board.side==WHITE)? ev : -ev;
        int mag = whiteAdv/150; if(mag>7)mag=7; if(mag<-7)mag=-7;
        litE = (mag>0)? mag:0;   // white advantage on top (E)
        litS = (mag<0)? -mag:0;  // black advantage on left (S)
    } else {
        long wtot=cfg.tc.baseMs>0?cfg.tc.baseMs:1;
        litE = (int)(7*clock.ms(chess::BLACK)/wtot);  // show both clocks as bars
        litS = (int)(7*clock.ms(chess::WHITE)/wtot);
        if(litE>7)litE=7; if(litS>7)litS=7;
    }
    for(int i=1;i<=7;i++){
        leds.set(hw::ledIndex(0,i), i<=litE? hw::RGB{80,80,80} : hw::RGB{0,0,0});
        leds.set(hw::ledIndex(i,0), i<=litS? hw::RGB{80,80,80} : hw::RGB{0,0,0});
    }
    // clock corner LEDs: active player's clock green
    leds.set(hw::ledIndex(8,0), board.side==chess::WHITE? hw::RGB{0,180,0}:hw::RGB{10,10,10}); // WCLK
    leds.set(hw::ledIndex(0,8), board.side==chess::BLACK? hw::RGB{0,180,0}:hw::RGB{10,10,10}); // BCLK
    leds.show();
}

} // namespace game
