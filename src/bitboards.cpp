#include "types.hpp"
#include <stdio.h>

const int BitTables[64] = {
    63, 30, 3, 32, 25, 41, 22, 33, 
    15, 50, 42, 13, 11, 53, 19, 34, 
    61, 29, 2, 51, 21, 43, 45, 10, 
    18, 47, 1, 54, 9, 57, 0, 35, 
    62, 31, 40, 4, 49, 5, 52, 26, 
    60, 6, 23, 44, 46, 27, 56, 16, 
    7, 39, 48, 24, 59, 14, 12, 55, 
    38, 28, 58, 20, 37, 17, 36, 8
};

int PopBit(U64 *bb) {
    U64 b = *bb;
    int idx = __builtin_ctzll(b);
    *bb = b & (b - 1);
    return idx;
}

int CountBit(U64 b) {
    return __builtin_popcountll(b);
}

void PrintBitBoard(U64 bb) {
    U64 shiftMe = 1ULL; //Empty bitboard with only the first bit set to 1
    int rank = 0;
    int file = 0;
    int sq = 0;
    int sq64 = 0;
    printf("\n");
    for(rank = RANK_8; rank >= RANK_1; --rank) {
        for(file = FILE_A; file <= FILE_H; ++file) {
            sq = FR2SQ(file, rank); // 120 based
            sq64 = SQ64(sq); // 64 based
            if((shiftMe << sq64) & bb) {
                printf("X");
            } else {
                printf("-");
            }
        }
        printf("\n");
    }
    printf("\n\n");
}
