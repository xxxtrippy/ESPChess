// Portable chess core (no Arduino deps) — usable on ESP32 and host tests.
#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace chess {

// Piece encoding: type in low 3 bits (1=P..6=K), colour bit = bit3 (0 white,1 black)
enum { EMPTY=0, P=1, N=2, B=3, R=4, Q=5, K=6 };
enum { WHITE=0, BLACK=1 };
inline int ptype(int pc){ return pc & 7; }
inline int pcolor(int pc){ return (pc>>3) & 1; }
inline int makePiece(int type,int color){ return type | (color<<3); }

// Castling-right bit flags
enum { CR_WK=1, CR_WQ=2, CR_BK=4, CR_BQ=8 };

// Move flags
enum { MF_NORMAL=0, MF_DOUBLE=1, MF_EP=2, MF_CASTLE=3 };

struct Move {
    uint8_t from=0, to=0;
    uint8_t promo=0;   // promotion piece type (N/B/R/Q) or 0
    uint8_t flag=0;    // MF_*
    bool operator==(const Move&o) const {
        return from==o.from&&to==o.to&&promo==o.promo&&flag==o.flag;
    }
};

struct Undo {
    Move m;
    uint8_t captured;  // piece on 'to' (or the EP-captured pawn)
    uint8_t castling;
    int8_t  ep;
    uint16_t halfmove;
    uint8_t  epCapSq;  // square cleared on EP
};

inline int fileOf(int sq){ return sq & 7; }
inline int rankOf(int sq){ return sq >> 3; }
inline int sqOf(int file,int rank){ return rank*8 + file; }

class Board {
public:
    int sq[64];
    int side;            // side to move
    uint8_t castling;
    int ep;              // en-passant target square or -1
    int halfmove;
    int fullmove;

    void clear();
    void setStart();
    bool setFEN(const std::string& fen);
    std::string toFEN() const;

    int kingSquare(int color) const;
    bool isAttacked(int square, int byColor) const;
    bool inCheck(int color) const;

    // pseudo-legal then filtered to legal
    void genPseudo(std::vector<Move>& out) const;
    void genLegal(std::vector<Move>& out) const;

    void makeMove(const Move& m, Undo& u);
    void unmakeMove(const Move& m, const Undo& u);

    // Does 'color' have any legal move?
    bool hasLegal(int color);

    // result helpers (standard chess)
    bool isCheckmate();   // side to move is checkmated
    bool isStalemate();   // side to move has no move and not in check
};

uint64_t perft(Board& b, int depth);

// algebraic helpers
std::string sqName(int sq);
int sqFromName(const std::string& s);
std::string moveUci(const Move& m);

} // namespace chess
