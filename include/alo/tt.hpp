#pragma once

#include "alo/types.hpp"

namespace alo {

// Lightweight OO wrapper around the existing transposition table C API.
// Allows gradual migration to method calls without breaking legacy code.
class TranspositionTable {
public:
    explicit TranspositionTable(S_HASHTABLE* table) : table_(table) {}

    void initDefault() { InitHashTable(table_); }
    void clear() { ClearHashTable(table_); }
    void reinitMB(int mb) { ReInitHashTable(table_, mb); }

    bool probe(Board* pos, int* move, int* score, int alpha, int beta, int depth) {
        return ProbeHashEntry(pos, move, score, alpha, beta, depth) == TRUE;
    }
    void store(Board* pos, int move, int score, int flags, int depth) {
        StoreHashEntry(pos, move, score, flags, depth);
    }
    int probePvMove(const Board* pos) { return ProbePvMove(pos); }
    int getPvLine(int depth, Board* pos) { return GetPvLine(depth, pos); }

    S_HASHTABLE* raw() { return table_; }
    const S_HASHTABLE* raw() const { return table_; }

private:
    S_HASHTABLE* table_;
};

} // namespace alo
