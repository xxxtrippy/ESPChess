// Physical 9x9 matrix <-> chess mapping, LED indexing, colours, brightness.
// Authoritative orientation (from project handoff):
//   row 0 = TOP (Black), row 8 = BOTTOM (White). (0,0)=M, (0,8)=BLACK CLK,
//   (8,0)=WHITE CLK, (0,1..7)=E1..E7, (1..7,0)=S1..S7, (1..8,1..8)=chess squares.
//   A1=(8,1), H8=(1,8).  square = rank*8 + file, file=col-1, rank=8-row.
#pragma once
#include <cstdint>
#include "chess.h"

namespace hw {

constexpr int ROWS=9, COLS=9, NLED=81;

enum Control { CTRL_NONE=0, CTRL_M, CTRL_E, CTRL_S, CTRL_WCLK, CTRL_BCLK };

struct RGB { uint8_t r,g,b; };

// (row,col) -> chess square 0..63, or -1 if not a chess square.
inline int matrixToSquare(int row,int col){
    if(row>=1&&row<=8&&col>=1&&col<=8){
        int file=col-1, rank=8-row;
        return rank*8+file;
    }
    return -1;
}
// chess square 0..63 -> (row,col)
inline void squareToMatrix(int square,int& row,int& col){
    int file=square&7, rank=square>>3;
    col=file+1; row=8-rank;
}
// control role of a (row,col); returns segment index 1..7 in 'idx' for E/S.
inline Control controlAt(int row,int col,int& idx){
    idx=0;
    if(row==0&&col==0) return CTRL_M;
    if(row==0&&col==8) return CTRL_BCLK;
    if(row==8&&col==0) return CTRL_WCLK;
    if(row==0&&col>=1&&col<=7){ idx=col; return CTRL_E; }
    if(col==0&&row>=1&&row<=7){ idx=row; return CTRL_S; }
    return CTRL_NONE;
}

// serpentine LED index (matches firmware + the hand-wiring diagram)
inline uint16_t ledIndex(int row,int col){
    return (row&1)==0 ? row*COLS+col : row*COLS+(COLS-1-col);
}
inline uint16_t squareToLed(int square){
    int r,c; squareToMatrix(square,r,c); return ledIndex(r,c);
}

// piece base colours (handoff)
inline RGB pieceColor(int type){
    switch(type){
        case chess::P: return {0,255,0};     // green
        case chess::N: return {160,0,255};   // purple
        case chess::R: return {255,140,0};   // amber
        case chess::B: return {0,80,255};     // blue
        case chess::Q: return {255,0,0};      // red
        case chess::K: return {255,255,0};    // yellow
    }
    return {0,0,0};
}
inline RGB scale(RGB c,uint8_t num,uint8_t den){
    return { (uint8_t)(c.r*num/den), (uint8_t)(c.g*num/den), (uint8_t)(c.b*num/den) };
}
// active player 100%, waiting 50%
inline RGB pieceRender(int piece,int sideToMove){
    RGB base=pieceColor(chess::ptype(piece));
    bool active = (chess::pcolor(piece)==sideToMove);
    return active ? base : scale(base,1,2);
}

} // namespace hw
