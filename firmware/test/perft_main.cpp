#include "chess.h"
#include <cstdio>
#include <vector>
#include <string>
using namespace chess;

struct Case { const char* fen; std::vector<std::pair<int,uint64_t>> depths; };

int main(){
    std::vector<Case> cases = {
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        {{1,20},{2,400},{3,8902},{4,197281},{5,4865609}}},
      {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // kiwipete
        {{1,48},{2,2039},{3,97862},{4,4085603}}},
      {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        {{1,14},{2,191},{3,2812},{4,43238},{5,674624}}},
      {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        {{1,6},{2,264},{3,9467}}},
      {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        {{1,44},{2,1486},{3,62379}}},
    };
    int fails=0, total=0;
    for(auto& c: cases){
        Board b; b.setFEN(c.fen);
        for(auto& d: c.depths){
            Board bb=b;
            uint64_t got=perft(bb, d.first);
            total++;
            bool ok = got==d.second;
            if(!ok) fails++;
            printf("perft(%d) %-60s got=%llu exp=%llu %s\n",
                   d.first, c.fen, (unsigned long long)got,
                   (unsigned long long)d.second, ok?"OK":"FAIL");
        }
    }
    printf("\nPERFT: %d/%d passed\n", total-fails, total);
    return fails? 1:0;
}
