#include "types.h"
#include <stdlib.h>

#define MATE 29000

static void CheckUp(SearchInfo *info) {
    if (info->timeset == TRUE && GetTimeMS() > info->stopTime) {
        info->stopped = TRUE;
    }
    ReadInput(info);
}

int IsRepetition(const Board *pos) {
    int index = 0;
    for (index = pos->hisPly - pos->fiftyMove; index < pos->hisPly - 1; ++index) {
        ASSERT(index >= 0 && index < MAX_GAME_MOVES);
        if (pos->posKey == pos->history[index].posKey) {
            return TRUE;
        }
    }
    return FALSE;
}

static void ClearForSearch(Board *pos, SearchInfo *info) {
    int index = 0;
    int index2 = 0;
    for (index = 0; index < 13; ++index) {
        for (index2 = 0; index2 < BOARD_SIZE; ++index2) {
            pos->searchHistory[index][index2] = 0;
        }
    }
    for (index = 0; index < 2; ++index) {
        for (index2 = 0; index2 < MAX_DEPTH; ++index2) {
            pos->searchKillers[index][index2] = 0;
        }
    }

    pos->HashTable->overWrite = 0;
    pos->ply = 0;

    info->stopped = 0;
    info->nodes = 0;

    info->fh = 0;
    info->fhf = 0;
}

static int AlphaBeta(int alpha, int beta, int depth, Board *pos, SearchInfo *info, int DoNull) {
    ASSERT(CheckBoard(pos));
    if (depth == 0) {
        info->nodes++;
        return EvaluatePosition(pos);
    }

    if ((info->nodes & 2047) == 0) {
        CheckUp(info);
    }

    info->nodes++;

    if ((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply) {
        return 0;
    }

    if (pos->ply > MAX_DEPTH - 1) {
        return EvaluatePosition(pos);
    }

    int inCheck = SqAttacked(pos->kingSq[pos->side], pos->side ^ 1, pos);

    if (inCheck) {
        depth++;
    }

    int score = -INFINITY;

    MoveList list[1];
    GenerateAllMoves(pos, list);

    int MoveNum = 0;
    int Legal = 0;
    int OldAlpha = alpha;
    int BestMove = NOMOVE;
    int BestScore = -INFINITY;

    score = -INFINITY;

    for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        if (!MakeMove(pos, list->moves[MoveNum].move)) {
            continue;
        }

        Legal++;
        int score;

        score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE);

        TakeMove(pos);
        if (info->stopped == TRUE) {
            return 0;
        }
        if (score > BestScore) {
            BestScore = score;
            BestMove = list->moves[MoveNum].move;
            if (score > alpha) {
                if (score >= beta) {
                    StoreHashEntry(pos, BestMove, beta, HFBETA, depth);
                    return beta;
                }
                alpha = score;
            }
        }
    }
    //No legal moves
    if (Legal == 0) {
        if (inCheck) {
            return -MATE + pos->ply;
        } else {
            return 0;
        }
    }
    
    //Store in hashtable
    if (alpha != OldAlpha) {
        StoreHashEntry(pos, BestMove, BestScore, HFEXACT, depth);
    } else {
        StoreHashEntry(pos, BestMove, alpha, HFALPHA, depth);
    }

    return alpha;
}

void SearchPosition(Board *pos, SearchInfo *info) {
    int bestMove = NOMOVE;
    int bestScore = -INFINITY;
    int currentDepth = 0;
    int pvMoves = 0;
    int pvNum = 0;

    ClearForSearch(pos, info);

    for (currentDepth = 1; currentDepth <= info->depth; ++currentDepth) {
        bestScore = AlphaBeta(-INFINITY, INFINITY, currentDepth, pos, info, TRUE);

        if (info->stopped == TRUE) {
            break;
        }

        pvMoves = GetPvLine(currentDepth, pos);
        bestMove = pos->PvArray[0];
        if (bestScore > 28500) {
            printf("info score mate %d depth %d nodes %ld time %d nps %d ", (29000 - bestScore) / 2, currentDepth, info->nodes,
                   (GetTimeMS() - info->startTime), (int)(info->nodes / ((GetTimeMS() - info->startTime) / 1000.0)));
        } else {
            printf("info score cp %d depth %d nodes %ld time %d nps %d ", bestScore, currentDepth, info->nodes, (GetTimeMS() - info->startTime),
                   (int)(info->nodes / ((GetTimeMS() - info->startTime) / 1000.0)));
        }
        pvMoves = GetPvLine(currentDepth, pos);
        printf("pv");
        for (pvNum = 0; pvNum < pvMoves; ++pvNum) {
            printf(" %s", PrMove(pos->PvArray[pvNum]));
        }
        printf("\n");
    }
    printf("bestmove %s\n", PrMove(bestMove));
}
