// Championship ladder: win advances a level, draw replays, loss restarts at 1.
#pragma once
namespace game {

struct Championship {
    int level=1;
    int minLevel=1, maxLevel=10;
    bool finished=false;

    void reset(){ level=minLevel; finished=false; }
    // Report the human's result of a match against the current level.
    void onResult(bool humanWon, bool draw){
        if(finished) return;
        if(draw) return;                      // replay same level
        if(humanWon){
            if(level>=maxLevel){ finished=true; }  // beat the top level
            else level++;
        } else {
            level=minLevel;                   // loss restarts at level 1
        }
    }
};

} // namespace game
