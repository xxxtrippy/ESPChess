// Virtual play-through demo: drives the SAME game core the firmware uses and
// prints full games, so you can see it actually play. Not a unit test.
#include "chess.h"
#include "variants.h"
#include "engine.h"
#include "game.h"
#include "board_map.h"
#include <cstdio>
#include <vector>
using namespace chess;
using namespace game;

static void printBoard(const Board& b){
    const char* g=" PNBRQK";
    for(int r=7;r>=0;r--){
        printf("  %d ", r+1);
        for(int f=0;f<8;f++){ int p=b.sq[sqOf(f,r)];
            char c = p==EMPTY? '.' : g[ptype(p)];
            if(p!=EMPTY && pcolor(p)==BLACK) c=(char)(c+32);
            printf("%c ", c);
        }
        printf("\n");
    }
    printf("    a b c d e f g h    (%s to move)\n", b.side==WHITE?"white":"black");
}

static const char* resName(Result r){
    switch(r){case WHITE_WIN:return "1-0 (White wins)";case BLACK_WIN:return "0-1 (Black wins)";
        case DRAW:return "1/2-1/2 (Draw)";default:return "ongoing";}
}

static void scriptedScholarsMate(){
    printf("\n================ DEMO 1: scripted human game (Scholar's mate) ================\n");
    Controller g; Config c; c.mode=STANDARD; c.tc=tc::Standard(); c.opponent=HUMAN_VS_HUMAN;
    g.newGame(c,0);
    const char* moves[]={"e2e4","e7e5","f1c4","b8c6","d1h5","g8f6","h5f7"};
    for(const char* mv: moves){
        int from=sqFromName(std::string(mv).substr(0,2));
        int to  =sqFromName(std::string(mv).substr(2,2));
        int r,cc; hw::squareToMatrix(from,r,cc); g.onCell(r,cc,0);
        hw::squareToMatrix(to,r,cc);             g.onCell(r,cc,0);
        g.onClock(g.board.side,0);
        printf("  played %s\n", mv);
    }
    printBoard(g.board);
    printf("  RESULT: %s  (checkmate=%d)\n", resName(g.result), g.board.isCheckmate());
}

static void aiSelfPlay(Mode mode,int level,uint32_t seed){
    printf("\n================ DEMO: AI vs AI self-play [%s, level %d] ================\n",
           modeName(mode), level);
    Controller g; Config c; c.mode=mode; c.tc=tc::Standard();
    c.opponent=AI_VS_AI; c.aiLevelWhite=level; c.aiLevelBlack=level; c.seed=seed;
    g.newGame(c,0);
    int ply=0;
    while(!g.gameOver() && ply<220){
        int mover=g.board.side;
        Move before; // capture move by diffing is messy; recompute via pickMove peek is not stored
        // We just let the controller move and read the last applied move from FEN change.
        std::string pre=g.board.toFEN();
        g.update(0);
        if(g.board.toFEN()==pre) break; // no progress
        ply++;
        if(ply<=12 || g.gameOver()){ /* show opening + end */ }
    }
    printBoard(g.board);
    printf("  plies=%d  RESULT: %s\n", ply, resName(g.result));
}

int main(){
    scriptedScholarsMate();
    aiSelfPlay(STANDARD, 4, 2024);
    aiSelfPlay(KING_OF_THE_HILL, 4, 7);
    aiSelfPlay(HORDE, 3, 11);
    printf("\nAll demos completed.\n");
    return 0;
}
