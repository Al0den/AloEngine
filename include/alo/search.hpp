#pragma once

#include "alo/types.hpp"
#include "alo/tt.hpp"

namespace alo {

class Searcher {
public:
    Searcher(Board* p, SearchInfo* i);
    void searchPosition();

    int searchScore(int depth, int time_ms = -1);
private:
    Board* pos;
    SearchInfo* info;
    TranspositionTable tt;
    void checkUp();
    void pickNextMove(int moveNum, MoveList *list);
    void clearForSearch();
    int quiescence(int alpha, int beta);
    int alphaBeta(int alpha, int beta, int depth, int doNull);

    
};

} // namespace alo

int SearchScore(Board *pos, SearchInfo *info, int depth, int time_ms = -1);

