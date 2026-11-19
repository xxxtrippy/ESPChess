#include "variants.h"
#include <vector>
#include <random>
using namespace chess;

namespace game {

const char* modeName(Mode m){
    switch(m){
        case STANDARD: return "Standard";
        case FOG_OF_WAR: return "Fog of War";
        case HORDE: return "Horde";
        case KING_OF_THE_HILL: return "King of the Hill";
        case PUZZLE: return "Puzzle";
        default: return "?";
    }
}

const char* startFEN(Mode m){
    switch(m){
        case HORDE:
            // Black standard; White 36-pawn horde (lichess start)
            return "rnbqkbnr/pppppppp/8/1PP2PP1/PPPPPPPP/PPPPPPPP/PPPPPPPP/PPPPPPPP w kq - 0 1";
        case STANDARD:
        case FOG_OF_WAR:
        case KING_OF_THE_HILL:
        case PUZZLE:
        default:
            return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }
}

// Build a random but legal puzzle position: both kings + a handful of pieces,
// side-to-move's opponent not left in check, side to move has a legal move.
static bool buildPuzzle(Board& b, uint32_t seed){
    std::mt19937 rng(seed? seed : 0xC0FFEE);
    for(int attempt=0; attempt<4000; ++attempt){
        b.clear();
        std::uniform_int_distribution<int> sqd(0,63);
        auto place=[&](int piece)->bool{
            for(int t=0;t<64;t++){ int s=sqd(rng);
                if(b.sq[s]==EMPTY && rankOf(s)!=0 && rankOf(s)!=7 ? true : b.sq[s]==EMPTY){
                    // allow pawns only on ranks 2..7 handled by caller
                    if(b.sq[s]==EMPTY){ b.sq[s]=piece; return true; }
                }
            }
            return false;
        };
        // kings
        int wk=sqd(rng), bk;
        b.sq[wk]=makePiece(K,WHITE);
        do{ bk=sqd(rng);}while(b.sq[bk]!=EMPTY ||
              (std::abs(fileOf(bk)-fileOf(wk))<=1 && std::abs(rankOf(bk)-rankOf(wk))<=1));
        b.sq[bk]=makePiece(K,BLACK);
        // random material
        std::uniform_int_distribution<int> cnt(2,6);
        int extra=cnt(rng);
        int types[5]={Q,R,B,N,P};
        for(int i=0;i<extra;i++){
            int color=rng()&1; int ty=types[rng()%5];
            for(int t=0;t<40;t++){ int s=sqd(rng);
                if(b.sq[s]!=EMPTY) continue;
                if(ty==P && (rankOf(s)==0||rankOf(s)==7)) continue;
                b.sq[s]=makePiece(ty,color); break;
            }
        }
        b.side = rng()&1; b.castling=0; b.ep=-1; b.halfmove=0; b.fullmove=1;
        // opponent must not be in check (illegal: side to move could capture king)
        if(b.inCheck(b.side^1)) continue;
        std::vector<Move> l; b.genLegal(l);
        if(l.empty()) continue;         // need a playable position
        return true;
    }
    b.setStart();
    return false;
}

void setupPosition(Board& b, Mode m, uint32_t seed){
    if(m==PUZZLE){ buildPuzzle(b,seed); return; }
    b.setFEN(startFEN(m));
}

static bool whiteHasPieces(const Board& b){
    for(int i=0;i<64;i++) if(b.sq[i]!=EMPTY && pcolor(b.sq[i])==WHITE) return true;
    return false;
}
static int countAll(const Board& b){
    int n=0; for(int i=0;i<64;i++) if(b.sq[i]!=EMPTY) n++; return n;
}
static bool insufficient(const Board& b){
    // K vs K, K vs KN, K vs KB only
    int minor=0, others=0;
    for(int i=0;i<64;i++){ int p=b.sq[i]; if(p==EMPTY)continue; int t=ptype(p);
        if(t==K) continue;
        if(t==N||t==B) minor++; else others++;
    }
    return others==0 && minor<=1;
}

Result result(Board& b, Mode m){
    // Mode-specific terminal checks first
    if(m==KING_OF_THE_HILL){
        int wk=b.kingSquare(WHITE), bk=b.kingSquare(BLACK);
        if(wk>=0 && isHill(wk)) return WHITE_WIN;
        if(bk>=0 && isHill(bk)) return BLACK_WIN;
    }
    if(m==HORDE){
        if(!whiteHasPieces(b)) return BLACK_WIN;   // all white pawns captured
    }
    std::vector<Move> l; b.genLegal(l);
    if(l.empty()){
        if(b.inCheck(b.side)) return (b.side==WHITE)?BLACK_WIN:WHITE_WIN; // checkmate
        return DRAW;                                                       // stalemate
    }
    if(b.halfmove>=100) return DRAW;        // 50-move rule
    if(m!=HORDE && insufficient(b)) return DRAW;
    return ONGOING;
}

bool fogReveals(const Board& b, Mode m, int viewer, int sq){
    if(m!=FOG_OF_WAR) return true;
    int p=b.sq[sq];
    if(p==EMPTY) return true;
    return pcolor(p)==viewer;   // only own pieces' identities are revealed
}

} // namespace game
