// Search/eval AI with 10 selectable strength levels (1=weakest .. 10=strongest).
#pragma once
#include "chess.h"
#include "variants.h"
#include <cstdint>

namespace ai {

struct SearchLimits { int depth; long nodeCap; };

int  evaluate(chess::Board& b, game::Mode mode);           // centipawns, stm perspective
int  search(chess::Board& b, game::Mode mode, int depth, int alpha, int beta,
            int ply, long& nodes, long nodeCap);

// Pick a move for the given strength level. rngState is advanced; deterministic
// for a fixed seed so tests reproduce. Returns false if no legal move.
bool pickMove(chess::Board& b, game::Mode mode, int level, uint32_t& rngState,
              chess::Move& out);

const char* levelName(int level);  // 1..10 descriptive label

} // namespace ai
