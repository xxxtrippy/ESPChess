#include "engine.h"
#include <vector>
#include <algorithm>
using namespace chess;

namespace ai {

static const int VAL[7] = {0,100,320,330,500,900,0}; // P N B R Q K
static const int MATE=30000, HILL=29000;

// piece-square (white perspective, a1=0). Light tables.
static const int PST_P[64]={
 0,0,0,0,0,0,0,0, 5,10,10,-20,-20,10,10,5, 5,-5,-10,0,0,-10,-5,5,
 0,0,0,20,20,0,0,0, 5,5,10,25,25,10,5,5, 10,10,20,30,30,20,10,10,
 50,50,50,50,50,50,50,50, 0,0,0,0,0,0,0,0 };
static const int PST_N[64]={
 -50,-40,-30,-30,-30,-30,-40,-50, -40,-20,0,5,5,0,-20,-40,
 -30,5,10,15,15,10,5,-30, -30,0,15,20,20,15,0,-30,
 -30,5,15,20,20,15,5,-30, -30,0,10,15,15,10,0,-30,
 -40,-20,0,0,0,0,-20,-40, -50,-40,-30,-30,-30,-30,-40,-50 };
static const int PST_B[64]={
 -20,-10,-10,-10,-10,-10,-10,-20, -10,5,0,0,0,0,5,-10,
 -10,10,10,10,10,10,10,-10, -10,0,10,10,10,10,0,-10,
 -10,5,5,10,10,5,5,-10, -10,0,5,10,10,5,0,-10,
 -10,0,0,0,0,0,0,-10, -20,-10,-10,-10,-10,-10,-10,-20 };
static const int PST_K[64]={
 20,30,10,0,0,10,30,20, 20,20,0,0,0,0,20,20, -10,-20,-20,-20,-20,-20,-20,-10,
 -20,-30,-30,-40,-40,-30,-30,-20, -30,-40,-40,-50,-50,-40,-40,-30,
 -30,-40,-40,-50,-50,-40,-40,-30, -30,-40,-40,-50,-50,-40,-40,-30,
 -30,-40,-40,-50,-50,-40,-40,-30 };

static int pst(int type,int sq,int color){
    int s = (color==WHITE)? sq : (56 ^ (sq & 56)) | (sq & 7); // mirror rank
    switch(type){ case P: return PST_P[s]; case N: return PST_N[s];
        case B: return PST_B[s]; case K: return PST_K[s]; default: return 0; }
}

int evaluate(Board& b, game::Mode mode){
    int sc=0;
    for(int i=0;i<64;i++){ int p=b.sq[i]; if(p==EMPTY)continue;
        int t=ptype(p), c=pcolor(p);
        int v=VAL[t]+pst(t,i,c);
        sc += (c==WHITE)? v : -v;
    }
    if(mode==game::KING_OF_THE_HILL){
        // nudge kings toward the centre
        auto cdist=[](int sq){ int f=fileOf(sq),r=rankOf(sq);
            return std::abs(2*f-7)+std::abs(2*r-7); };
        int wk=b.kingSquare(WHITE), bk=b.kingSquare(BLACK);
        if(wk>=0) sc += (14-cdist(wk))*6;
        if(bk>=0) sc -= (14-cdist(bk))*6;
    }
    return (b.side==WHITE)? sc : -sc;
}

static int terminalHill(Board& b){
    int wk=b.kingSquare(WHITE), bk=b.kingSquare(BLACK);
    if(wk>=0 && game::isHill(wk)) return (b.side==WHITE)? HILL : -HILL;
    if(bk>=0 && game::isHill(bk)) return (b.side==BLACK)? HILL : -HILL;
    return 0;
}

static int mvvlva(const Board& b,const Move& m){
    int cap=b.sq[m.to]; int att=ptype(b.sq[m.from]);
    int c = (cap==EMPTY)?0:VAL[ptype(cap)];
    return c*16 - att;
}

int search(Board& b, game::Mode mode, int depth, int alpha, int beta,
           int ply, long& nodes, long nodeCap){
    nodes++;
    if(mode==game::KING_OF_THE_HILL){
        int th=terminalHill(b);
        if(th!=0) return (th>0)? (HILL-ply) : -(HILL-ply);
    }
    if(depth<=0 || nodes>nodeCap) return evaluate(b,mode);
    std::vector<Move> l; b.genLegal(l);
    if(l.empty()){
        if(b.inCheck(b.side)) return -(MATE-ply);   // checkmated
        if(mode==game::HORDE && b.side==WHITE) return -(MATE-ply); // white stuck ~ loss-ish
        return 0;                                    // stalemate => draw
    }
    std::sort(l.begin(),l.end(),[&](const Move&a,const Move&c){
        return mvvlva(b,a)>mvvlva(b,c); });
    int best=-MATE-1;
    for(auto& m: l){
        Undo u; b.makeMove(m,u);
        int sc=-search(b,mode,depth-1,-beta,-alpha,ply+1,nodes,nodeCap);
        b.unmakeMove(m,u);
        if(sc>best) best=sc;
        if(sc>alpha) alpha=sc;
        if(alpha>=beta) break;
    }
    return best;
}

static uint32_t xr(uint32_t& s){ s^=s<<13; s^=s>>17; s^=s<<5; return s; }

static void levelParams(int level,int& depth,int& blunderPct){
    if(level<1)level=1; if(level>10)level=10;
    static const int D[11]  ={0, 1,1,2,2,3,3,3,4,4,5};
    static const int BL[11] ={0,100,60,45,30,20,12,6,3,0,0};
    depth=D[level]; blunderPct=BL[level];
}

bool pickMove(Board& b, game::Mode mode, int level, uint32_t& rng, Move& out){
    std::vector<Move> l; b.genLegal(l);
    if(l.empty()) return false;
    int depth,blunder; levelParams(level,depth,blunder);
    if((int)(xr(rng)%100) < blunder){        // blunder: random legal move
        out=l[xr(rng)%l.size()]; return true;
    }
    std::sort(l.begin(),l.end(),[&](const Move&a,const Move&c){
        return mvvlva(b,a)>mvvlva(b,c); });
    long nodes=0, cap=400000;
    int best=-MATE-2; std::vector<Move> bestMoves;
    for(auto& m: l){
        Undo u; b.makeMove(m,u);
        int sc=-search(b,mode,depth-1,-MATE-2,MATE+2,1,nodes,cap);
        b.unmakeMove(m,u);
        if(sc>best){ best=sc; bestMoves.clear(); bestMoves.push_back(m); }
        else if(sc==best) bestMoves.push_back(m);
    }
    out = bestMoves[xr(rng)%bestMoves.size()];  // tie-break randomly for variety
    return true;
}

const char* levelName(int level){
    static const char* N[11]={"?",
        "L1 Newborn","L2 Beginner","L3 Casual","L4 Club","L5 Intermediate",
        "L6 Strong","L7 Expert","L8 Master","L9 Elite","L10 Max"};
    if(level<1)level=1; if(level>10)level=10; return N[level];
}

} // namespace ai
