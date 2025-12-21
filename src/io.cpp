#include "alo/types.hpp"
// Implementation for alo::Io
#include <stdio.h>
#include "alo/io.hpp"

namespace alo {
char* Io::formatSquare(const int sq) {
        static char SqStr[3];
        int file = FilesBrd[sq];
        int rank = RanksBrd[sq];
        snprintf(SqStr, sizeof(SqStr), "%c%c", ('a' + file), ('1' + rank));
        return SqStr;
}

char* Io::formatMove(const int move) {
        static char MvStr[6];
        int ff = FilesBrd[FROMSQ(move)];
        int rf = RanksBrd[FROMSQ(move)];
        int ft = FilesBrd[TOSQ(move)];
        int rt = RanksBrd[TOSQ(move)];
        int promoted = PROMOTED(move);
        if(promoted) {
            char pchar = 'q';
            if(IsKn(promoted))       pchar = 'n';
            else if(IsRQ(promoted) && !IsBQ(promoted)) pchar = 'r';
            else if(!IsRQ(promoted) && IsBQ(promoted)) pchar = 'b';
            snprintf(MvStr, sizeof(MvStr), "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);
        } else {
            snprintf(MvStr, sizeof(MvStr), "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));
        }
        return MvStr;
}

int Io::parseMove(const char *ptrChar, Board *pos) {
        if(ptrChar[1] > '8' || ptrChar[1] < '1') return NOMOVE;
        if(ptrChar[3] > '8' || ptrChar[3] < '1') return NOMOVE;
        if(ptrChar[0] > 'h' || ptrChar[0] < 'a') return NOMOVE;
        if(ptrChar[2] > 'h' || ptrChar[2] < 'a') return NOMOVE;
        int from = FR2SQ(ptrChar[0] - 'a', ptrChar[1] - '1');
        int to = FR2SQ(ptrChar[2] - 'a', ptrChar[3] - '1');
        ASSERT(SqOnBoard(from) && SqOnBoard(to));
        MoveList list[1];
        GenerateAllMoves(pos, list);
        int MoveNum = 0;
        int Move = 0;
        int PromPce = EMPTY;
        for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
            Move = list->moves[MoveNum].move;
            if(FROMSQ(Move) == from && TOSQ(Move) == to) {
                PromPce = PROMOTED(Move);
                if(PromPce != EMPTY) {
                    if(IsRQ(PromPce) && !IsBQ(PromPce) && ptrChar[4] == 'r') return Move;
                    else if(!IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4] == 'b') return Move;
                    else if(IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4] == 'q') return Move;
                    else if(IsKn(PromPce) && ptrChar[4] == 'n') return Move;
                    continue;
                }
                return Move;
            }
        }
        return NOMOVE;
}

void Io::printMoveList(MoveList *list) {
        printf("MoveList:\n");
        for(int index = 0; index < list->count; ++index) {
            int move = list->moves[index].move;
            int score = list->moves[index].score;
            printf("Move:%d > %s (score:%d)\n", index+1, formatMove(move), score);
        }
        printf("MoveList Total %d Moves:\n\n", list->count);
}

void Io::printBin(int move) {
        printf("As binary:");
        for(int index = 27; index >= 0; --index) {
            if((1 << index) & move) printf("1"); else printf("0");
            if(index != 26 && index % 4 == 0) printf(" ");
        }
}
} // namespace alo

// C API wrappers
char *PrSq(const int sq) { return alo::Io::formatSquare(sq); }
char *PrMove(const int move) { return alo::Io::formatMove(move); }
int ParseMove(const char *ptrChar, Board *pos) { return alo::Io::parseMove(ptrChar, pos); }
void PrintMoveList(MoveList *list) { alo::Io::printMoveList(list); }
void PrintBin(int move) { alo::Io::printBin(move); }
