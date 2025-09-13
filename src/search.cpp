#include "types.hpp"
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

static void PickNextMove(int moveNum, MoveList *list) {
    S_MOVE temp;
    int index = 0;
    int bestScore = 0;
    int bestNum = moveNum;
    for (index = moveNum; index < list->count; ++index) {
        if (list->moves[index].score > bestScore) {
            bestScore = list->moves[index].score;
            bestNum = index;
        }
    }
    ASSERT(moveNum >= 0 && moveNum < list->count);
    ASSERT(bestNum >= 0 && bestNum < list->count);
    ASSERT(bestNum >= moveNum);
    temp = list->moves[moveNum];
    list->moves[moveNum] = list->moves[bestNum];
    list->moves[bestNum] = temp;
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

static int Quiescence(int alpha, int beta, Board *pos, SearchInfo *info) {
    ASSERT(CheckBoard(pos));
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
    int score = EvaluatePosition(pos);
    if (score >= beta) {
        return beta;
    }
    if (score > alpha) {
        alpha = score;
    }

    MoveList list[1];
    GenerateAllCaptures(pos, list);

    int MoveNum = 0;
    int Legal = 0;
    int OldAlpha = alpha;
    int BestMove = NOMOVE;
    int BestScore = -SCORE_INF;
    score = -SCORE_INF;
    for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        PickNextMove(MoveNum, list);
        if (!MakeMove(pos, list->moves[MoveNum].move)) {
            continue;
        }
        Legal++;
        score = -Quiescence(-beta, -alpha, pos, info);
        TakeMove(pos);
        if (info->stopped == TRUE) {
            return 0;
        }
        if (score > alpha) {
            if (score >= beta) {
                return beta;
            }
            alpha = score;
            BestMove = list->moves[MoveNum].move;
        }
    }

    return alpha;
}

static int AlphaBeta(int alpha, int beta, int depth, Board *pos, SearchInfo *info, int DoNull) {
    ASSERT(CheckBoard(pos));
    if (depth == 0) {
        info->nodes++;
        return Quiescence(alpha, beta, pos, info);
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

    int score = -SCORE_INF;
    int PvMove = NOMOVE;

    if (ProbeHashEntry(pos, &PvMove, &score, alpha, beta, depth) == TRUE) {
        return score;
    }

    //Null Move Pruning
    if (DoNull && !inCheck && pos->ply && (pos->bigPce[pos->side] > 0) && depth >= 6) {
        MakeNullMove(pos);
        score = -AlphaBeta(-beta, -beta + 1, depth - 6, pos, info, FALSE);
        TakeNullMove(pos);
        if (info->stopped == TRUE) {
            return 0;
        }
        if (score >= beta && abs(score) < ISMATE) {
            return beta;
        }
    }


    MoveList list[1];
    GenerateAllMoves(pos, list);

    int MoveNum = 0;
    int Legal = 0;
    int OldAlpha = alpha;
    int BestMove = NOMOVE;
    int BestScore = -SCORE_INF;

    score = -SCORE_INF;

    //PvMoves priority & ordering
    if(PvMove != NOMOVE) {
        for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
            if(list->moves[MoveNum].move == PvMove) {
                list->moves[MoveNum].score = 2000000;
                break;
            }
        }
    }

    for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        PickNextMove(MoveNum, list);
        if (!MakeMove(pos, list->moves[MoveNum].move)) {
            continue;
        }

        Legal++;
        int score;

        // Late Move Reductions: reduce depth for late, non-capture, non-check moves
        int reduction = 0;
        if (depth >= 3 && MoveNum > 3 && !(list->moves[MoveNum].move & (MFLAGCAP | MFLAGPROM)) && !inCheck) {
            reduction = 1;
        }

        score = -AlphaBeta(-beta, -alpha, depth - 1 - reduction, pos, info, TRUE);

        // If reduced and improved, re-search at full depth
        if (reduction && score > alpha) {
            score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE);
        }

        TakeMove(pos);
        if (info->stopped == TRUE) {
            return 0;
        }
        if (score > BestScore) {
            BestScore = score;
            BestMove = list->moves[MoveNum].move;
            if (score > alpha) {
                if (score >= beta) {
                    if (!(list->moves[MoveNum].move & MFLAGCAP)) {
                        pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
                        pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
                    }

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
    int bestScore = -SCORE_INF;
    int currentDepth = 0;
    int pvMoves = 0;
    int pvNum = 0;

    ClearForSearch(pos, info);

    for (currentDepth = 1; currentDepth <= info->depth; ++currentDepth) {
        bestScore = AlphaBeta(-SCORE_INF, SCORE_INF, currentDepth, pos, info, TRUE);

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
