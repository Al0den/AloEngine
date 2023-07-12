#include "types.h"
#include <stdlib.h>

int GetPvLine(const int depth, Board *pos) {
    ASSERT(depth < MAXDEPTH && depth >= 1);

    int move = ProbePvMove(pos);
    int count = 0;

    while (move != NOMOVE && count < depth) {
        if (MoveExists(pos, move)) {
            MakeMove(pos, move);
            pos->PvArray[count++] = move;
        } else {
            break;
        }
        move = ProbePvMove(pos);
    }

    while (pos->ply > 0) {
        TakeMove(pos);
    }

    return count;
}

const int HashSize = 0x100000 * 1024;

void ClearHashTable(S_HASHTABLE *table) {

    S_HASHENTRY *tableEntry;

    for (tableEntry = table->pTable; tableEntry < table->pTable + table->numEntries; tableEntry++) {
        tableEntry->posKey = 0ULL;
        tableEntry->move = NOMOVE;
        tableEntry->depth = 0;
        tableEntry->score = 0;
        tableEntry->flags = 0;
    }
    table->newWrite = 0;
}

void InitHashTable(S_HASHTABLE *table) {
    table->numEntries = HashSize / sizeof(S_HASHENTRY);
    table->numEntries -= 2;

    table->pTable = (S_HASHENTRY *)malloc(table->numEntries * sizeof(S_HASHENTRY));
    if (table->pTable == NULL) {
        printf("Hash Allocation Failed");
    } else {
        ClearHashTable(table);
        printf("HashTable init complete with %d entries\n", table->numEntries);
    }
}

int ProbeHashEntry(Board *pos, int *move, int *score, int alpha, int beta, int depth) {
    int index = pos->posKey % pos->HashTable->numEntries;
    if (pos->HashTable->pTable[index].posKey == pos->posKey) {
        *move = pos->HashTable->pTable[index].move;
        if (pos->HashTable->pTable[index].depth >= depth) {
            *score = pos->HashTable->pTable[index].score;
            if (*score > ISMATE) {
                *score -= pos->ply;
            } else if (*score < -ISMATE) {
                *score += pos->ply;
            }
            switch (pos->HashTable->pTable[index].flags) {
            case HFALPHA:
                if (*score <= alpha) {
                    *score = alpha;
                    return TRUE;
                }
                break;
            case HFBETA:
                if (*score >= beta) {
                    *score = beta;
                    return TRUE;
                }
                break;
            case HFEXACT:
                return TRUE;
                break;
            default:
                ASSERT(FALSE);
                break;
            }
        }
    }
    return FALSE;
}

void StoreHashEntry(Board *pos, const int move, int score, const int flags, const int depth) {
    int index = pos->posKey % pos->HashTable->numEntries;
    if (pos->HashTable->pTable[index].posKey == 0) {
        pos->HashTable->newWrite++;
    } else {
        //Ignore if new depth is lower than previous one
        if(pos->HashTable->pTable[index].depth > depth) {
            return;
        }
        pos->HashTable->overWrite++;
    }

    if (score > ISMATE) {
        score += pos->ply;
    } else if (score < -ISMATE) {
        score -= pos->ply;
    }

    pos->HashTable->pTable[index].move = move;
    pos->HashTable->pTable[index].posKey = pos->posKey;
    pos->HashTable->pTable[index].flags = flags;
    pos->HashTable->pTable[index].score = score;
    pos->HashTable->pTable[index].depth = depth;
}

int ProbePvMove(const Board *pos) {
    int index = pos->posKey % pos->HashTable->numEntries;
    ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);

    if (pos->HashTable->pTable[index].posKey == pos->posKey) {
        return pos->HashTable->pTable[index].move;
    }

    return NOMOVE;
}
