#include "alo/types.hpp"
#include <stdlib.h>

int GetPvLine(const int depth, Board *pos) {
    ASSERT(depth < MAX_DEPTH && depth >= 1);

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

const int HashSize = 0x100000 * 256; // bytes

void ClearHashTable(S_HASHTABLE *table) {
    for (int i = 0; i < table->numEntries; ++i) {
        table->pTable[i].posKey = 0ULL;
        table->pTable[i].move = NOMOVE;
        table->pTable[i].depth = 0;
        table->pTable[i].score = 0;
        table->pTable[i].flags = 0;
    }
    table->newWrite = 0;
}

static int highestPowerOfTwo(int x);

static void freeTable(S_HASHTABLE* table) {
    if (table && table->pTable) {
        free(table->pTable);
        table->pTable = NULL;
    }
    table->numEntries = 0;
    table->mask = 0;
    table->newWrite = 0;
    table->overWrite = 0;
}

static int highestPowerOfTwo(int x) {
    int p = 1;
    while (p << 1 <= x) p <<= 1;
    return p;
}

void InitHashTable(S_HASHTABLE *table) {
    int entries = HashSize / (int)sizeof(S_HASHENTRY);
    table->numEntries = highestPowerOfTwo(entries);
    table->mask = (U64)(table->numEntries - 1);

    void* mem = nullptr;
    if (posix_memalign(&mem, 64, table->numEntries * sizeof(S_HASHENTRY)) != 0) {
        mem = malloc(table->numEntries * sizeof(S_HASHENTRY));
    }
    table->pTable = (S_HASHENTRY*)mem;
    if (table->pTable == NULL) {
        #ifdef DEBUG
        printf("Hash Allocation Failed");
        #endif
        table->numEntries = 0;
        table->mask = 0;
        return;
    }
    ClearHashTable(table);
    #ifdef DEBUG
    printf("HashTable init complete with %d entries\n", table->numEntries);
    #endif
}

// Re-initialize hash table with a given size in megabytes.
// Frees previous allocation (if any) and resizes.
void ReInitHashTable(S_HASHTABLE *table, int megabytes) {
    if (megabytes < 1) megabytes = 1;
    int targetBytes = megabytes * 1024 * 1024;
    freeTable(table);
    int entries = targetBytes / (int)sizeof(S_HASHENTRY);
    if (entries < 1) entries = 1;
    table->numEntries = highestPowerOfTwo(entries);
    table->mask = (U64)(table->numEntries - 1);

    void* mem = nullptr;
    if (posix_memalign(&mem, 64, table->numEntries * sizeof(S_HASHENTRY)) != 0) {
        mem = malloc(table->numEntries * sizeof(S_HASHENTRY));
    }
    table->pTable = (S_HASHENTRY*)mem;
    if (table->pTable == NULL) {
        table->numEntries = 0;
        table->mask = 0;
        return;
    }
    ClearHashTable(table);
}

int ProbeHashEntry(Board *pos, int *move, int *score, int alpha, int beta, int depth) {
    U64 index = pos->posKey & pos->HashTable->mask;
    __builtin_prefetch(&pos->HashTable->pTable[index]);
    if (pos->HashTable->pTable[index].posKey == pos->posKey) {
        *move = pos->HashTable->pTable[index].move;
        if (pos->HashTable->pTable[index].depth >= depth) {
            *score = (int)pos->HashTable->pTable[index].score;
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
    U64 index = pos->posKey & pos->HashTable->mask;
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
    if (score > 32767) score = 32767; else if (score < -32768) score = -32768;
    pos->HashTable->pTable[index].score = (short)score;
    pos->HashTable->pTable[index].depth = depth;
}

int ProbePvMove(const Board *pos) {
    U64 index = pos->posKey & pos->HashTable->mask;
    ASSERT(index >= 0 && index <= pos->HashTable->numEntries - 1);

    if (pos->HashTable->pTable[index].posKey == pos->posKey) {
        return pos->HashTable->pTable[index].move;
    }

    return NOMOVE;
}
