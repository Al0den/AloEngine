#pragma once

#include "alo/types.hpp"

namespace alo {

class MoveGenerator {
public:
    static void generateAll(Board *pos, MoveList *list);
    static void generateCaptures(Board *pos, MoveList *list);
    static int moveExists(Board *pos, const int move);
};

} // namespace alo

