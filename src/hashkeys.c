#include "types.h"

 U64 GeneratePosKey(const Board* pos) {
    int sq = 0;
    U64 finalKey = 0;
    int piece = EMPTY;
    for(sq = 0; sq < BOARD_SIZE; ++sq) {
        piece = pos->pieces[sq];
        if(piece != NO_SQ && piece != EMPTY && piece != OFFBOARD) {
            ASSERT(piece >= wP && piece <= bK);
            finalKey ^= PieceKeys[piece][sq];
        }
    }
    if(pos->side == WHITE) {
        finalKey ^= SideKey;
    }
    if(pos->enPas != NO_SQ) {
        ASSERT(pos->enPas >= 0 && pos->enPas < BOARD_SIZE);
        finalKey ^= PieceKeys[EMPTY][pos->enPas];
    }
    ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);
    finalKey ^= CastleKeys[pos->castlePerm];
    return finalKey;
 }
