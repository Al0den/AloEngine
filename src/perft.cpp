#include "alo/types.hpp"

long leafNodes = 0;

void Perft(int depth, Board *pos) {
    ASSERT(CheckBoard(pos));
    if(depth == 0) {
        leafNodes++;
        return;
    }
    MoveList list[1];
    GenerateAllMoves(pos, list);
    int moveNum = 0;
    for(moveNum = 0; moveNum < list->count; ++moveNum) {
        if(!MakeMove(pos, list->moves[moveNum].move)) {
            continue;
        }
        Perft(depth - 1, pos);
        TakeMove(pos);
    }
    return;
}

void PerftTest(int depth, Board *pos) {
    ASSERT(CheckBoard(pos));
    PrintBoard(pos);
    printf("\nStarting Test To Depth:%d\n", depth);
    leafNodes = 0;
    MoveList list[1];
    GenerateAllMoves(pos, list);
    int move;
    int moveNum = 0;
    for(moveNum = 0; moveNum < list->count; ++moveNum) {
        move = list->moves[moveNum].move;
        if(!MakeMove(pos, move)) {
            printf("In check");
            continue;
        }
        long cumnodes = leafNodes;
        Perft(depth - 1, pos);
        TakeMove(pos);
        long oldnodes = leafNodes - cumnodes;
        printf("move %d : %s : %ld\n", moveNum + 1, PrMove(move), oldnodes);
    }
    printf("\nTest Complete : %ld nodes visited\n", leafNodes);
    return;
}
