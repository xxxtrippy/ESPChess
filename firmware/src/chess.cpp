#include "chess.h"
#include <sstream>
#include <cctype>

namespace chess {

static const int KN[8] = {17,15,10,6,-17,-15,-10,-6};
static const int KO[8] = {1,-1,8,-8,9,7,-9,-7};
static const int BISH[4] = {9,7,-9,-7};
static const int ROOK[4] = {1,-1,8,-8};

// file distance guard for offset moves on a 0..63 board
static inline bool onBoard(int sq){ return sq>=0 && sq<64; }

void Board::clear(){
    for(int i=0;i<64;i++) sq[i]=EMPTY;
    side=WHITE; castling=0; ep=-1; halfmove=0; fullmove=1;
}

void Board::setStart(){
    setFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

static int charToPiece(char c){
    int color = std::isupper((unsigned char)c) ? WHITE : BLACK;
    switch(std::tolower((unsigned char)c)){
        case 'p': return makePiece(P,color);
        case 'n': return makePiece(N,color);
        case 'b': return makePiece(B,color);
        case 'r': return makePiece(R,color);
        case 'q': return makePiece(Q,color);
        case 'k': return makePiece(K,color);
    }
    return EMPTY;
}
static char pieceToChar(int pc){
    const char* s=" pnbrqk";
    char c = s[ptype(pc)];
    if(pcolor(pc)==WHITE) c = std::toupper((unsigned char)c);
    return c;
}

bool Board::setFEN(const std::string& fen){
    clear();
    std::istringstream ss(fen);
    std::string board, stm, cr, eps; int hm=0, fm=1;
    if(!(ss>>board)) return false;
    ss>>stm>>cr>>eps; ss>>hm>>fm;
    int rank=7, file=0;
    for(char c: board){
        if(c=='/'){ rank--; file=0; }
        else if(std::isdigit((unsigned char)c)){ file += c-'0'; }
        else { if(rank<0||file>7) return false; sq[sqOf(file,rank)] = charToPiece(c); file++; }
    }
    side = (stm=="b") ? BLACK : WHITE;
    castling=0;
    for(char c: cr){
        if(c=='K') castling|=CR_WK; else if(c=='Q') castling|=CR_WQ;
        else if(c=='k') castling|=CR_BK; else if(c=='q') castling|=CR_BQ;
    }
    ep = (eps=="-"||eps.empty()) ? -1 : sqFromName(eps);
    halfmove=hm; fullmove=fm;
    return true;
}

std::string Board::toFEN() const {
    std::ostringstream o;
    for(int rank=7;rank>=0;rank--){
        int empty=0;
        for(int file=0;file<8;file++){
            int pc=sq[sqOf(file,rank)];
            if(pc==EMPTY) empty++;
            else { if(empty){o<<empty;empty=0;} o<<pieceToChar(pc); }
        }
        if(empty) o<<empty;
        if(rank) o<<'/';
    }
    o<<' '<<(side==WHITE?'w':'b')<<' ';
    std::string cr;
    if(castling&CR_WK)cr+='K'; if(castling&CR_WQ)cr+='Q';
    if(castling&CR_BK)cr+='k'; if(castling&CR_BQ)cr+='q';
    o<<(cr.empty()?"-":cr)<<' ';
    o<<(ep<0?std::string("-"):sqName(ep))<<' '<<halfmove<<' '<<fullmove;
    return o.str();
}

int Board::kingSquare(int color) const {
    int tgt = makePiece(K,color);
    for(int i=0;i<64;i++) if(sq[i]==tgt) return i;
    return -1;
}

bool Board::isAttacked(int square, int byColor) const {
    int f=fileOf(square), r=rankOf(square);
    // pawns
    int dir = (byColor==WHITE)? -1 : +1; // attacker pawn sits one rank toward its home
    for(int df : {-1,1}){
        int af=f+df, ar=r+dir;
        if(af>=0&&af<8&&ar>=0&&ar<8){
            int p=sq[sqOf(af,ar)];
            if(p!=EMPTY && pcolor(p)==byColor && ptype(p)==P) return true;
        }
    }
    // knights
    static const int ndf[8]={1,2,2,1,-1,-2,-2,-1};
    static const int ndr[8]={2,1,-1,-2,-2,-1,1,2};
    for(int k=0;k<8;k++){
        int af=f+ndf[k], ar=r+ndr[k];
        if(af>=0&&af<8&&ar>=0&&ar<8){
            int p=sq[sqOf(af,ar)];
            if(p!=EMPTY&&pcolor(p)==byColor&&ptype(p)==N) return true;
        }
    }
    // king
    for(int df=-1;df<=1;df++)for(int dr=-1;dr<=1;dr++){
        if(!df&&!dr)continue;
        int af=f+df,ar=r+dr;
        if(af>=0&&af<8&&ar>=0&&ar<8){
            int p=sq[sqOf(af,ar)];
            if(p!=EMPTY&&pcolor(p)==byColor&&ptype(p)==K) return true;
        }
    }
    // sliding: bishop/queen diagonals
    static const int bdf[4]={1,1,-1,-1}, bdr[4]={1,-1,1,-1};
    for(int d=0;d<4;d++){
        int af=f+bdf[d], ar=r+bdr[d];
        while(af>=0&&af<8&&ar>=0&&ar<8){
            int p=sq[sqOf(af,ar)];
            if(p!=EMPTY){
                if(pcolor(p)==byColor&&(ptype(p)==B||ptype(p)==Q)) return true;
                break;
            }
            af+=bdf[d]; ar+=bdr[d];
        }
    }
    // sliding: rook/queen orthogonals
    static const int rdf[4]={1,-1,0,0}, rdr[4]={0,0,1,-1};
    for(int d=0;d<4;d++){
        int af=f+rdf[d], ar=r+rdr[d];
        while(af>=0&&af<8&&ar>=0&&ar<8){
            int p=sq[sqOf(af,ar)];
            if(p!=EMPTY){
                if(pcolor(p)==byColor&&(ptype(p)==R||ptype(p)==Q)) return true;
                break;
            }
            af+=rdf[d]; ar+=rdr[d];
        }
    }
    return false;
}

bool Board::inCheck(int color) const {
    int ks=kingSquare(color);
    if(ks<0) return false;
    return isAttacked(ks, color^1);
}

static void addPawnMoves(std::vector<Move>& out,int from,int to,int flag){
    int rank=rankOf(to);
    if(rank==0||rank==7){
        for(int pr : {Q,R,B,N}){ Move m; m.from=from;m.to=to;m.promo=pr;m.flag=flag; out.push_back(m);}
    } else { Move m; m.from=from;m.to=to;m.flag=flag; out.push_back(m); }
}

void Board::genPseudo(std::vector<Move>& out) const {
    out.clear();
    int me=side;
    for(int s=0;s<64;s++){
        int pc=sq[s]; if(pc==EMPTY||pcolor(pc)!=me) continue;
        int t=ptype(pc), f=fileOf(s), r=rankOf(s);
        if(t==P){
            int dir = (me==WHITE)? 1 : -1;
            int startRank = (me==WHITE)? 1 : 6;
            int one = sqOf(f, r+dir);
            if(r+dir>=0&&r+dir<8&&sq[one]==EMPTY){
                addPawnMoves(out,s,one,MF_NORMAL);
                if(r==startRank){
                    int two=sqOf(f,r+2*dir);
                    if(sq[two]==EMPTY){ Move m;m.from=s;m.to=two;m.flag=MF_DOUBLE; out.push_back(m); }
                }
            }
            for(int df : {-1,1}){
                int cf=f+df, cr=r+dir;
                if(cf<0||cf>7||cr<0||cr>7) continue;
                int cs=sqOf(cf,cr);
                if(sq[cs]!=EMPTY && pcolor(sq[cs])!=me){ addPawnMoves(out,s,cs,MF_NORMAL); }
                else if(cs==ep && ep>=0){ Move m;m.from=s;m.to=cs;m.flag=MF_EP; out.push_back(m); }
            }
        } else if(t==N){
            static const int ndf[8]={1,2,2,1,-1,-2,-2,-1};
            static const int ndr[8]={2,1,-1,-2,-2,-1,1,2};
            for(int k=0;k<8;k++){
                int af=f+ndf[k],ar=r+ndr[k];
                if(af<0||af>7||ar<0||ar>7)continue;
                int ts=sqOf(af,ar);
                if(sq[ts]==EMPTY||pcolor(sq[ts])!=me){ Move m;m.from=s;m.to=ts; out.push_back(m);}
            }
        } else if(t==K){
            for(int df=-1;df<=1;df++)for(int dr=-1;dr<=1;dr++){
                if(!df&&!dr)continue;
                int af=f+df,ar=r+dr;
                if(af<0||af>7||ar<0||ar>7)continue;
                int ts=sqOf(af,ar);
                if(sq[ts]==EMPTY||pcolor(sq[ts])!=me){ Move m;m.from=s;m.to=ts; out.push_back(m);}
            }
            // castling
            if(me==WHITE && s==sqOf(4,0)){
                if((castling&CR_WK)&&sq[sqOf(5,0)]==EMPTY&&sq[sqOf(6,0)]==EMPTY
                    && !isAttacked(sqOf(4,0),BLACK)&&!isAttacked(sqOf(5,0),BLACK)&&!isAttacked(sqOf(6,0),BLACK))
                    { Move m;m.from=s;m.to=sqOf(6,0);m.flag=MF_CASTLE; out.push_back(m);}
                if((castling&CR_WQ)&&sq[sqOf(3,0)]==EMPTY&&sq[sqOf(2,0)]==EMPTY&&sq[sqOf(1,0)]==EMPTY
                    && !isAttacked(sqOf(4,0),BLACK)&&!isAttacked(sqOf(3,0),BLACK)&&!isAttacked(sqOf(2,0),BLACK))
                    { Move m;m.from=s;m.to=sqOf(2,0);m.flag=MF_CASTLE; out.push_back(m);}
            } else if(me==BLACK && s==sqOf(4,7)){
                if((castling&CR_BK)&&sq[sqOf(5,7)]==EMPTY&&sq[sqOf(6,7)]==EMPTY
                    && !isAttacked(sqOf(4,7),WHITE)&&!isAttacked(sqOf(5,7),WHITE)&&!isAttacked(sqOf(6,7),WHITE))
                    { Move m;m.from=s;m.to=sqOf(6,7);m.flag=MF_CASTLE; out.push_back(m);}
                if((castling&CR_BQ)&&sq[sqOf(3,7)]==EMPTY&&sq[sqOf(2,7)]==EMPTY&&sq[sqOf(1,7)]==EMPTY
                    && !isAttacked(sqOf(4,7),WHITE)&&!isAttacked(sqOf(3,7),WHITE)&&!isAttacked(sqOf(2,7),WHITE))
                    { Move m;m.from=s;m.to=sqOf(2,7);m.flag=MF_CASTLE; out.push_back(m);}
            }
        } else {
            const int* dirs; int nd;
            if(t==B){dirs=BISH;nd=4;} else if(t==R){dirs=ROOK;nd=4;} else {dirs=nullptr;nd=8;}
            static const int qdf[8]={1,1,-1,-1,1,-1,0,0};
            static const int qdr[8]={1,-1,1,-1,0,0,1,-1};
            static const int bdf[4]={1,1,-1,-1}, bdr[4]={1,-1,1,-1};
            static const int rdf[4]={1,-1,0,0}, rdr[4]={0,0,1,-1};
            const int *DF,*DR; int D;
            if(t==B){DF=bdf;DR=bdr;D=4;} else if(t==R){DF=rdf;DR=rdr;D=4;} else {DF=qdf;DR=qdr;D=8;}
            for(int d=0;d<D;d++){
                int af=f+DF[d],ar=r+DR[d];
                while(af>=0&&af<8&&ar>=0&&ar<8){
                    int ts=sqOf(af,ar);
                    if(sq[ts]==EMPTY){ Move m;m.from=s;m.to=ts; out.push_back(m);}
                    else { if(pcolor(sq[ts])!=me){Move m;m.from=s;m.to=ts; out.push_back(m);} break; }
                    af+=DF[d]; ar+=DR[d];
                }
            }
        }
    }
}

void Board::makeMove(const Move& m, Undo& u){
    u.m=m; u.castling=castling; u.ep=ep; u.halfmove=halfmove; u.captured=EMPTY; u.epCapSq=255;
    int pc=sq[m.from]; int me=pcolor(pc); int t=ptype(pc);
    int newEp=-1;
    halfmove++;
    if(t==P) halfmove=0;
    if(m.flag==MF_EP){
        int capSq = sqOf(fileOf(m.to), rankOf(m.from));
        u.captured=sq[capSq]; u.epCapSq=capSq; sq[capSq]=EMPTY;
        halfmove=0;
    } else if(sq[m.to]!=EMPTY){
        u.captured=sq[m.to]; halfmove=0;
    }
    sq[m.to]=pc; sq[m.from]=EMPTY;
    if(m.promo) sq[m.to]=makePiece(m.promo, me);
    if(m.flag==MF_DOUBLE){ newEp = sqOf(fileOf(m.from),(rankOf(m.from)+rankOf(m.to))/2); }
    if(m.flag==MF_CASTLE){
        int rank=rankOf(m.from);
        if(fileOf(m.to)==6){ sq[sqOf(5,rank)]=sq[sqOf(7,rank)]; sq[sqOf(7,rank)]=EMPTY; }
        else { sq[sqOf(3,rank)]=sq[sqOf(0,rank)]; sq[sqOf(0,rank)]=EMPTY; }
    }
    // update castling rights
    if(t==K){ if(me==WHITE) castling&=~(CR_WK|CR_WQ); else castling&=~(CR_BK|CR_BQ); }
    auto touch=[&](int s){
        if(s==sqOf(0,0)) castling&=~CR_WQ;
        else if(s==sqOf(7,0)) castling&=~CR_WK;
        else if(s==sqOf(0,7)) castling&=~CR_BQ;
        else if(s==sqOf(7,7)) castling&=~CR_BK;
    };
    touch(m.from); touch(m.to);
    ep=newEp;
    if(side==BLACK) fullmove++;
    side^=1;
}

void Board::unmakeMove(const Move& m, const Undo& u){
    side^=1;
    if(side==BLACK) fullmove--;
    int me=side;
    int moved = sq[m.to];
    if(m.promo) moved = makePiece(P, me);
    sq[m.from]=moved;
    sq[m.to]=EMPTY;
    if(m.flag==MF_EP){
        sq[u.epCapSq]=u.captured;
    } else if(u.captured!=EMPTY){
        sq[m.to]=u.captured;
    }
    if(m.flag==MF_CASTLE){
        int rank=rankOf(m.from);
        if(fileOf(m.to)==6){ sq[sqOf(7,rank)]=sq[sqOf(5,rank)]; sq[sqOf(5,rank)]=EMPTY; }
        else { sq[sqOf(0,rank)]=sq[sqOf(3,rank)]; sq[sqOf(3,rank)]=EMPTY; }
    }
    castling=u.castling; ep=u.ep; halfmove=u.halfmove;
}

void Board::genLegal(std::vector<Move>& out) const {
    std::vector<Move> ps; genPseudo(ps);
    out.clear();
    Board& self = const_cast<Board&>(*this);
    int me=side;
    for(auto& m: ps){
        Undo u; self.makeMove(m,u);
        int k=self.kingSquare(me);
        bool attacked = (k>=0) && self.isAttacked(k, me^1); // no king (Horde) => never "in check"
        if(!attacked) out.push_back(m);
        self.unmakeMove(m,u);
    }
}

bool Board::hasLegal(int color){
    if(side!=color) return true; // only defined for side to move in this use
    std::vector<Move> l; genLegal(l); return !l.empty();
}

bool Board::isCheckmate(){
    std::vector<Move> l; genLegal(l);
    return l.empty() && inCheck(side);
}
bool Board::isStalemate(){
    std::vector<Move> l; genLegal(l);
    return l.empty() && !inCheck(side);
}

uint64_t perft(Board& b, int depth){
    if(depth==0) return 1;
    std::vector<Move> l; b.genLegal(l);
    if(depth==1) return l.size();
    uint64_t nodes=0;
    for(auto& m: l){ Undo u; b.makeMove(m,u); nodes+=perft(b,depth-1); b.unmakeMove(m,u); }
    return nodes;
}

std::string sqName(int sq){ std::string s; s+=('a'+fileOf(sq)); s+=('1'+rankOf(sq)); return s; }
int sqFromName(const std::string& s){
    if(s.size()<2) return -1;
    int f=s[0]-'a', r=s[1]-'1';
    if(f<0||f>7||r<0||r>7) return -1;
    return sqOf(f,r);
}
std::string moveUci(const Move& m){
    std::string s=sqName(m.from)+sqName(m.to);
    if(m.promo){ const char* c=" pnbrqk"; s+=c[m.promo]; }
    return s;
}

} // namespace chess
