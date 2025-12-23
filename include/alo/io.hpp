#pragma once

#include "alo/types.hpp"

namespace alo {

class Io {
public:
    static char* formatSquare(const int sq);
    static char* formatMove(const int move);
    static int parseMove(const char *ptrChar, Board *pos);
    static void printMoveList(MoveList *list);
    static void printBin(int move);
};

} // namespace alo

