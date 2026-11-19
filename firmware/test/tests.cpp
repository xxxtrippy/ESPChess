#include "chess.h"
#include "board_map.h"
#include "variants.h"
#include "clock.h"
#include "engine.h"
#include "game.h"
#include "championship.h"
#include "menu.h"
#include "sim.h"
#include <cstdio>
#include <vector>
#include <set>
#include <string>
using namespace chess;
using namespace game;

static int g_pass=0, g_fail=0;
#define CHECK(cond,msg) do{ if(cond){g_pass++;} else {g_fail++; \
    printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__);} }while(0)

static void sect(const char* s){ printf("[%s]\n", s); }

// ---- helpers ----
static void sqToCell(int sq,int& row,int& col){ hw::squareToMatrix(sq,row,col); }
static void humanMove(Controller& g,int from,int to,long now){
    int r,c; sqToCell(from,r,c); g.onCell(r,c,now);
    sqToCell(to,r,c);   g.onCell(r,c,now);
    g.onClock(g.board.side, now);       // active player presses own clock to commit
}

// ================= PERFT (subset, fast) =================
static void testPerft(){
    sect("perft");
    struct C{const char* fen; int d; uint64_t n;};
    std::vector<C> cs={
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",4,197281},
      {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",3,97862},
      {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",4,43238},
      {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",3,9467},
      {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",3,62379},
    };
    for(auto&c:cs){ Board b; b.setFEN(c.fen); uint64_t g=perft(b,c.d);
        CHECK(g==c.n, (std::string("perft ")+c.fen).c_str()); }
}

// ================= MAPPING =================
static void testMapping(){
    sect("mapping");
    CHECK(hw::matrixToSquare(8,1)==sqFromName("a1"), "A1=(8,1)");
    CHECK(hw::matrixToSquare(1,8)==sqFromName("h8"), "H8=(1,8)");
    CHECK(hw::matrixToSquare(8,8)==sqFromName("h1"), "H1=(8,8)");
    CHECK(hw::matrixToSquare(1,1)==sqFromName("a8"), "A8=(1,1)");
    CHECK(hw::matrixToSquare(0,0)==-1, "control (0,0) not a square");
    // inverse round-trip
    bool rt=true; for(int s=0;s<64;s++){int r,c;hw::squareToMatrix(s,r,c);
        if(hw::matrixToSquare(r,c)!=s)rt=false;}
    CHECK(rt,"square<->matrix round trip");
    // controls
    int idx; CHECK(hw::controlAt(0,0,idx)==hw::CTRL_M,"M");
    CHECK(hw::controlAt(0,8,idx)==hw::CTRL_BCLK,"BCLK");
    CHECK(hw::controlAt(8,0,idx)==hw::CTRL_WCLK,"WCLK");
    CHECK(hw::controlAt(0,3,idx)==hw::CTRL_E&&idx==3,"E3");
    CHECK(hw::controlAt(3,0,idx)==hw::CTRL_S&&idx==3,"S3");
    // ledIndex is a permutation of 0..80
    std::set<int> seen; for(int r=0;r<9;r++)for(int c=0;c<9;c++)seen.insert(hw::ledIndex(r,c));
    CHECK(seen.size()==81 && *seen.begin()==0 && *seen.rbegin()==80,"ledIndex permutation 0..80");
    CHECK(hw::ledIndex(0,0)==0 && hw::ledIndex(0,8)==8 && hw::ledIndex(1,8)==9
          && hw::ledIndex(1,0)==17 && hw::ledIndex(8,8)==80,"serpentine endpoints");
    // colours (handoff)
    CHECK(hw::pieceColor(P).g==255 && hw::pieceColor(Q).r==255 && hw::pieceColor(K).r==255
          && hw::pieceColor(K).g==255,"piece colours");
    // brightness: waiting side at 50%
    hw::RGB active=hw::pieceRender(makePiece(P,WHITE),WHITE);
    hw::RGB wait  =hw::pieceRender(makePiece(P,BLACK),WHITE);
    CHECK(active.g==255 && wait.g==127,"active 100% / waiting 50%");
}

// ================= CLOCK =================
static void testClock(){
    sect("clock");
    Clock ck; ck.configure(tc::Blitz(), 0);
    CHECK(ck.ms(WHITE)==5*60000 && ck.active==WHITE,"blitz init");
    ck.update(2000); CHECK(ck.ms(WHITE)==5*60000-2000,"white ticks down");
    bool ok=ck.press(WHITE,2000); CHECK(ok && ck.active==BLACK,"white press -> black");
    CHECK(ck.ms(WHITE)==5*60000-2000,"blitz no increment");
    ck.update(3000); CHECK(ck.ms(BLACK)==5*60000-1000,"black ticks after switch");
    CHECK(ck.press(WHITE,3000)==false,"wrong clock does nothing");
    // increment
    Clock r; r.configure(tc::Rapid(),0); r.update(1000); r.press(WHITE,1000);
    CHECK(r.ms(WHITE)==15*60000-1000+10000,"rapid +10s increment");
    // expiration
    Clock e; e.configure(tc::Bullet(),0); e.update(60001);
    CHECK(e.expired() && e.flagged()==WHITE,"flag on timeout");
}

// ================= RULES / variants =================
static void testRules(){
    sect("rules");
    { Board b; b.setFEN("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
      CHECK(b.isCheckmate(),"fool's-mate position is checkmate"); }
    { Board b; b.setFEN("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
      CHECK(b.isStalemate(),"stalemate detected"); }
    { // castling executes
      Board b; b.setFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
      std::vector<Move> l; b.genLegal(l); bool castle=false; Move cm;
      for(auto&m:l) if(m.flag==MF_CASTLE && m.to==sqFromName("g1")){castle=true;cm=m;}
      CHECK(castle,"white O-O available");
      if(castle){Undo u;b.makeMove(cm,u);
        CHECK(b.sq[sqFromName("g1")]==makePiece(K,WHITE)&&b.sq[sqFromName("f1")]==makePiece(R,WHITE),
              "O-O moved king+rook");}
    }
    { // promotion
      Board b; b.setFEN("8/P7/8/8/8/8/8/k6K w - - 0 1");
      std::vector<Move> l; b.genLegal(l); bool promoQ=false;
      for(auto&m:l) if(m.promo==Q) promoQ=true;
      CHECK(promoQ,"pawn promotion generated"); }
    { // en passant
      Board b; b.setFEN("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
      std::vector<Move> l; b.genLegal(l); bool ep=false;
      for(auto&m:l) if(m.flag==MF_EP) ep=true;
      CHECK(ep,"en passant available"); }
}

static void testVariants(){
    sect("variants");
    // KotH win when a king reaches the hill
    { Board b; b.setFEN("8/8/8/4K3/8/8/8/7k w - - 0 1"); // white king e5 (hill)
      CHECK(game::result(b,KING_OF_THE_HILL)==WHITE_WIN,"KotH white king on hill wins"); }
    { Board b; b.setFEN("8/8/8/4K3/8/8/8/6qk w - - 0 1"); // king on hill, but material present
      CHECK(game::result(b,STANDARD)==ONGOING,"king on e5 is ongoing in standard (no hill rule)"); }
    // Horde: start position legal & ongoing; white missing -> black win
    { Board b; setupPosition(b,HORDE);
      int wp=0; for(int i=0;i<64;i++) if(b.sq[i]!=EMPTY&&pcolor(b.sq[i])==WHITE) wp++;
      CHECK(wp==36 && b.kingSquare(WHITE)<0,"horde: 36 white pawns, no white king");
      CHECK(game::result(b,HORDE)==ONGOING,"horde start ongoing"); }
    { Board b; b.setFEN("4k3/8/8/8/8/8/8/8 b - - 0 1"); // no white pieces
      CHECK(game::result(b,HORDE)==BLACK_WIN,"horde: all white captured -> black wins"); }
    // Fog of war reveal rule
    { Board b; b.setStart();
      CHECK(game::fogReveals(b,FOG_OF_WAR,WHITE,sqFromName("a1"))==true,"fog: own piece revealed");
      CHECK(game::fogReveals(b,FOG_OF_WAR,WHITE,sqFromName("a8"))==false,"fog: enemy hidden");
      CHECK(game::fogReveals(b,STANDARD,WHITE,sqFromName("a8"))==true,"standard: all revealed"); }
    // Puzzle: random legal position
    { bool allgood=true; for(uint32_t s=1;s<=20;s++){ Board b; setupPosition(b,PUZZLE,s);
        std::vector<Move> l; b.genLegal(l);
        if(l.empty()||b.kingSquare(WHITE)<0||b.kingSquare(BLACK)<0||b.inCheck(b.side^1)) allgood=false; }
      CHECK(allgood,"puzzle: 20 seeds give legal, playable positions"); }
}

// ================= AI tactics =================
static void testAI(){
    sect("ai");
    // mate in 1 found at top level
    { Board b; b.setFEN("6k1/R7/6K1/8/8/8/8/8 w - - 0 1");
      uint32_t rng=1; Move m; bool ok=ai::pickMove(b,STANDARD,10,rng,m);
      Undo u; b.makeMove(m,u);
      CHECK(ok && b.isCheckmate(),"level 10 finds mate-in-1"); }
    // weak vs strong: level 10 should score >= level 1 material over a quick game set
    { int strongWins=0, games=6;
      for(int gi=0; gi<games; gi++){
        Board b; b.setStart(); uint32_t rng=100+gi; Result res=ONGOING; int ply=0;
        while(ply<160){ res=game::result(b,STANDARD); if(res!=ONGOING) break;
          int level = (b.side==WHITE)? 8 : 1;   // strong white vs weak black
          Move m; if(!ai::pickMove(b,STANDARD,level,rng,m)) break;
          Undo u; b.makeMove(m,u); ply++; }
        if(res==WHITE_WIN) strongWins++;
      }
      CHECK(strongWins>=4,"strong (L8) beats weak (L1) in most games"); }
}

// ================= SIMULATED PLAY (full control path) =================
static void testPlayFoolsMate(){
    sect("play: fool's mate via controller");
    Controller g; Config c; c.mode=STANDARD; c.tc=tc::Standard(); c.opponent=HUMAN_VS_HUMAN;
    g.newGame(c,0);
    SimLeds leds; g.render(leds);
    CHECK(leds.shows==1 && leds.litCount()>0,"initial render lights board");
    // 1. f3 e5 2. g4 Qh4#
    humanMove(g, sqFromName("f2"), sqFromName("f3"), 0);
    humanMove(g, sqFromName("e7"), sqFromName("e5"), 0);
    humanMove(g, sqFromName("g2"), sqFromName("g4"), 0);
    CHECK(!g.gameOver(),"not over before mating move");
    humanMove(g, sqFromName("d8"), sqFromName("h4"), 0);
    CHECK(g.gameOver() && g.result==BLACK_WIN,"fool's mate -> black wins");
    g.render(leds);  // render terminal position without crashing
    CHECK(leds.shows>=2,"renders after game end");
}

static void testPlayHumanVsAI(){
    sect("play: human (random) vs AI to completion");
    Controller g; Config c; c.mode=STANDARD; c.tc=tc::Standard();
    c.opponent=HUMAN_VS_AI; c.aiColor=BLACK; c.aiLevel=3; c.seed=7;
    g.newGame(c,0);
    uint32_t hrng=42; int ply=0;
    while(!g.gameOver() && ply<300){
        if(g.board.side==WHITE){ // human: pick a random legal move, play via cells+clock
            std::vector<Move> l; g.board.genLegal(l);
            if(l.empty()) break;
            Move m=l[(hrng=hrng*1103515245+12345, (hrng>>16))%l.size()];
            humanMove(g, m.from, m.to, 0);
        } else {
            g.update(0); // AI moves
        }
        ply++;
    }
    CHECK(ply<300,"human-vs-AI game terminates");
    printf("    -> result=%d after %d plies\n", g.result, ply);
    CHECK(true,"no crash in human-vs-AI path");
}

static void testSelfPlay(){
    sect("play: AI self-play (standard + KotH)");
    for(int t=0;t<2;t++){
        Mode mode = t==0? STANDARD : KING_OF_THE_HILL;
        Controller g; Config c; c.mode=mode; c.tc=tc::Standard();
        c.opponent=AI_VS_AI; c.aiLevelWhite=3; c.aiLevelBlack=3; c.seed=99+t;
        g.newGame(c,0);
        SimLeds leds; int ply=0;
        while(!g.gameOver() && ply<250){ g.update(0); g.render(leds); ply++; }
        CHECK(leds.count()==81,"led strip length 81");
        CHECK(ply<250 || g.gameOver(),"self-play terminates or caps safely");
        printf("    -> %s ended result=%d after %d plies\n", modeName(mode), g.result, ply);
    }
    CHECK(true,"self-play completed without crash");
}

static void testTimedGameFlagging(){
    sect("play: timed game can flag");
    Controller g; Config c; c.mode=STANDARD; c.tc=tc::Bullet();
    c.opponent=HUMAN_VS_HUMAN; g.newGame(c,0);
    g.update(60001);           // white runs out of time
    CHECK(g.clock.expired() && g.clock.flagged()==WHITE,"white flags on time");
    CHECK(g.gameOver(),"flagged game is over");
}

// advance one ply: human side plays a random legal move, AI side auto-moves.
static bool advancePly(Controller& g, uint32_t& rng){
    if(g.gameOver()) return false;
    bool humanTurn = (g.cfg.opponent==HUMAN_VS_HUMAN)
        || (g.cfg.opponent==HUMAN_VS_AI && g.board.side!=g.cfg.aiColor);
    if(humanTurn){
        std::vector<Move> l; g.board.genLegal(l);
        if(l.empty()) return false;
        Move m=l[(rng=rng*1103515245u+12345u,(rng>>16))%l.size()];
        humanMove(g, m.from, m.to, 0);
    } else g.update(0);
    return true;
}

static void testMenu24(){
    sect("menu: all 24 configurations");
    bool mapOK=true, runOK=true;
    for(int gt=0; gt<GT_COUNT; gt++){
        // mapping sanity
        Config c = buildConfig((GameType)gt, OPP_AI, 5, BLACK, 1);
        if(gt==GT_RAPID && !(c.tc.baseMs==15*60000 && c.tc.incMs==10000)) mapOK=false;
        if(gt==GT_BLITZ && !(c.tc.baseMs==5*60000)) mapOK=false;
        if(gt==GT_BULLET&& !(c.tc.baseMs==1*60000)) mapOK=false;
        if(gt==GT_HORDE && c.mode!=HORDE) mapOK=false;
        if(gt==GT_KOTH  && c.mode!=KING_OF_THE_HILL) mapOK=false;
        if(gt==GT_FOG   && c.mode!=FOG_OF_WAR) mapOK=false;
        if(gt==GT_PUZZLE&& c.mode!=PUZZLE) mapOK=false;
        for(int op=0; op<OPP_COUNT; op++){
            Session s; s.gameType=(GameType)gt; s.opp=(OppMode)op; s.aiLevel=2; s.seed=100+gt*3+op;
            s.start(0);
            if(!s.inGame) runOK=false;
            uint32_t rng=7+gt*op+1;
            SimLeds leds;
            for(int p=0;p<8 && !s.ctrl.gameOver(); ++p){ advancePly(s.ctrl,rng); s.ctrl.render(leds); }
            if(leds.count()!=81) runOK=false;
        }
    }
    CHECK(mapOK,"game-type -> mode/time-control mapping correct for all 8");
    CHECK(runOK,"all 8x3=24 configs start and run without crashing");
}

static void testChampionshipSession(){
    sect("championship session (human vs 10 AIs)");
    Session s; s.gameType=GT_STANDARD; s.opp=OPP_CHAMPIONSHIP; s.humanColor=WHITE;
    s.start(0);
    CHECK(s.champ.level==1 && s.inGame,"championship starts at level 1");
    // forced outcomes exercise the ladder transitions deterministically
    s.ctrl.result=WHITE_WIN; s.onGameEnd(0); CHECK(s.champ.level==2,"human win advances to L2");
    s.ctrl.result=DRAW;      s.onGameEnd(0); CHECK(s.champ.level==2,"draw rematches same level");
    s.ctrl.result=BLACK_WIN; s.onGameEnd(0); CHECK(s.champ.level==1,"human loss restarts at L1");
    // climb all the way: force 10 human wins
    int guard=0; while(!s.champ.finished && guard++<30){ s.ctrl.result=WHITE_WIN; s.onGameEnd(0); }
    CHECK(s.champ.finished && s.champ.level==10,"beating all 10 completes the ladder");
    // one real championship game plays to a natural result without crashing
    Session r; r.gameType=GT_KOTH; r.opp=OPP_CHAMPIONSHIP; r.humanColor=WHITE; r.seed=55;
    r.start(0); uint32_t rng=3; int ply=0;
    while(!r.ctrl.gameOver() && ply<220){ advancePly(r.ctrl,rng); ply++; }
    r.onGameEnd(0);
    CHECK(ply<220,"a real championship (KotH) game terminates");
}

static void testChampionship(){
    sect("championship ladder");
    Championship ch; ch.reset();
    CHECK(ch.level==1,"starts at level 1");
    ch.onResult(true,false);  CHECK(ch.level==2,"win advances");
    ch.onResult(false,true);  CHECK(ch.level==2,"draw replays same level");
    ch.onResult(false,false); CHECK(ch.level==1,"loss restarts at 1");
    for(int i=0;i<20 && !ch.finished;i++) ch.onResult(true,false);
    CHECK(ch.level==10 && ch.finished,"winning through level 10 finishes the ladder");
}

int main(){
    printf("==== Chessboard firmware test suite ====\n");
    testPerft();
    testMapping();
    testClock();
    testRules();
    testVariants();
    testAI();
    testPlayFoolsMate();
    testPlayHumanVsAI();
    testSelfPlay();
    testTimedGameFlagging();
    testChampionship();
    testMenu24();
    testChampionshipSession();
    printf("\n==== RESULT: %d passed, %d failed ====\n", g_pass, g_fail);
    return g_fail? 1:0;
}
