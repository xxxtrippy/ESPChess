// Game variants: start positions and win conditions.
#pragma once
#include "chess.h"

namespace game {

enum Mode { STANDARD=0, FOG_OF_WAR, HORDE, KING_OF_THE_HILL, PUZZLE, MODE_COUNT };

enum Result { ONGOING=0, WHITE_WIN, BLACK_WIN, DRAW };

const char* modeName(Mode m);
const char* startFEN(Mode m);           // base FEN for a mode (PUZZLE: a default)
void setupPosition(chess::Board& b, Mode m, uint32_t seed=0);

// King-of-the-Hill centre squares d4,e4,d5,e5
inline bool isHill(int sq){ return sq==27||sq==28||sq==35||sq==36; }

// Evaluate end state for the side to move / board in the given mode.
Result result(chess::Board& b, Mode m);

// Fog of war: should 'viewer' see the true identity of the piece on 'sq'?
// (handoff rule: board visible, enemy identities hidden)
bool fogReveals(const chess::Board& b, Mode m, int viewer, int sq);

} // namespace game
