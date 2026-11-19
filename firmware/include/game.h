// Firmware-agnostic game controller: matrix input -> chess logic -> LED output.
#pragma once
#include "chess.h"
#include "variants.h"
#include "clock.h"
#include "board_map.h"
#include "hal.h"
#include <cstdint>

namespace game {

enum Opponent { HUMAN_VS_HUMAN=0, HUMAN_VS_AI, AI_VS_AI };

struct Config {
    Mode mode=STANDARD;
    TimeControl tc{"Standard",0,0,true};
    Opponent opponent=HUMAN_VS_HUMAN;
    int aiColor=chess::BLACK;   // which colour the AI plays in HUMAN_VS_AI
    int aiLevel=5;
    int aiLevelWhite=5, aiLevelBlack=5; // for AI_VS_AI
    uint32_t seed=0xC0FFEE;
};

class Controller {
public:
    chess::Board board;
    Clock clock;
    Config cfg;
    Result result=ONGOING;
    uint32_t rng=0xC0FFEE;

    // selection / staging
    int selFrom=-1;            // selected source square
    int stagedFrom=-1, stagedTo=-1, stagedPromo=chess::Q;

    void newGame(const Config& c, long now);
    // A chess-square or control switch press at physical (row,col).
    void onCell(int row, int col, long now);
    // A clock button press by 'player' (0 white / 1 black).
    void onClock(int player, long now);
    // Periodic: ticks clock, runs AI when it is the AI's turn.
    void update(long now);
    // Render full board + highlights + segments to the LEDs.
    void render(hal::ILeds& leds);

    bool gameOver() const { return result!=ONGOING || clock.expired(); }
    int  currentPlayer() const { return board.side; }

private:
    bool legalTarget(int from,int to) const;
    bool commitStaged(long now);
    void doAIMove(long now);
    bool isAITurn() const;
    void refreshResult();
};

} // namespace game
