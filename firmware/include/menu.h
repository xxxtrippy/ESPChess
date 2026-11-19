// Menu model: the 8 game types x 3 opponent modes = 24 configurations,
// exactly as specified:
//   Game types (8): Standard, Rapid, Blitz, Bullet, Puzzle, Fog of War, Horde,
//                   King of the Hill.
//   Opponent (3):   2 Humans, Single AI (selectable level), Championship (climb
//                   all 10 AI levels; loss=restart at 1, draw=rematch same level).
#pragma once
#include "game.h"
#include "championship.h"

namespace game {

enum GameType {
    GT_STANDARD=0, GT_RAPID, GT_BLITZ, GT_BULLET,
    GT_PUZZLE, GT_FOG, GT_HORDE, GT_KOTH, GT_COUNT
};
enum OppMode { OPP_HUMAN=0, OPP_AI, OPP_CHAMPIONSHIP, OPP_COUNT };

inline const char* gameTypeName(GameType g){
    switch(g){
        case GT_STANDARD: return "Standard";
        case GT_RAPID:    return "Rapid 15|10";
        case GT_BLITZ:    return "Blitz 5|0";
        case GT_BULLET:   return "Bullet 1|0";
        case GT_PUZZLE:   return "Puzzle";
        case GT_FOG:      return "Fog of War";
        case GT_HORDE:    return "Horde";
        case GT_KOTH:     return "King of the Hill";
        default:          return "?";
    }
}
inline const char* oppName(OppMode o){
    switch(o){ case OPP_HUMAN: return "2 Players";
        case OPP_AI: return "Vs AI"; case OPP_CHAMPIONSHIP: return "Championship";
        default: return "?"; }
}

inline Mode modeOf(GameType g){
    switch(g){
        case GT_PUZZLE: return PUZZLE;
        case GT_FOG:    return FOG_OF_WAR;
        case GT_HORDE:  return HORDE;
        case GT_KOTH:   return KING_OF_THE_HILL;
        default:        return STANDARD;   // Standard/Rapid/Blitz/Bullet
    }
}
inline TimeControl tcOf(GameType g){
    switch(g){
        case GT_RAPID:  return tc::Rapid();
        case GT_BLITZ:  return tc::Blitz();
        case GT_BULLET: return tc::Bullet();
        default:        return tc::Standard();  // variants + Standard are untimed
    }
}

// Build a Controller config for a chosen cell of the 24-grid. For championship,
// pass the current ladder level in aiLevel.
inline Config buildConfig(GameType g, OppMode o, int aiLevel, int aiColor, uint32_t seed){
    Config c;
    c.mode=modeOf(g); c.tc=tcOf(g); c.seed=seed;
    if(o==OPP_HUMAN){ c.opponent=HUMAN_VS_HUMAN; }
    else { c.opponent=HUMAN_VS_AI; c.aiColor=aiColor; c.aiLevel=aiLevel; }
    return c;
}

// Top-level session: holds the menu selection, the live game, and (for
// championship) the ladder. One place the firmware and tests both drive.
struct Session {
    GameType gameType=GT_STANDARD;
    OppMode  opp=OPP_HUMAN;
    int      aiLevel=5;          // for OPP_AI
    int      humanColor=chess::WHITE;
    uint32_t seed=0xC0FFEE;

    Controller ctrl;
    Championship champ;
    bool inGame=false;

    void menuNextType(){ gameType=(GameType)((gameType+1)%GT_COUNT); }
    void menuNextOpp(){  opp=(OppMode)((opp+1)%OPP_COUNT); }
    void menuLevelUp(){  aiLevel = aiLevel>=10?1:aiLevel+1; }

    int activeAiLevel() const { return opp==OPP_CHAMPIONSHIP ? champ.level : aiLevel; }
    int aiColor() const { return humanColor^1; }

    void start(long now){
        if(opp==OPP_CHAMPIONSHIP) champ.reset();
        Config c=buildConfig(gameType, opp, activeAiLevel(), aiColor(), seed);
        ctrl.newGame(c, now);
        inGame=true;
    }
    // Call once when a game finishes. Returns true if a NEW game was started
    // (championship advancing/retrying); false if the session is idle/complete.
    bool onGameEnd(long now){
        if(opp!=OPP_CHAMPIONSHIP){ inGame=false; return false; }
        Result r=ctrl.result;
        bool draw = (r==DRAW);
        bool humanWon = (r==WHITE_WIN && humanColor==chess::WHITE)
                      ||(r==BLACK_WIN && humanColor==chess::BLACK);
        champ.onResult(humanWon, draw);
        if(champ.finished){ inGame=false; return false; }  // ladder complete
        // start the next match at the (possibly changed) level
        Config c=buildConfig(gameType, OPP_CHAMPIONSHIP, champ.level, aiColor(), seed+champ.level);
        ctrl.newGame(c, now);
        inGame=true;
        return true;
    }
};

} // namespace game
