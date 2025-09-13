#pragma once

#include "stdio.h"
#include "stdlib.h"

#define BOARD_SIZE 120
#define MAX_GAME_MOVES 2048
#define MAX_POSITION_MOVES 256
#define MAX_DEPTH 64

// Scoring constants
constexpr int SCORE_INF = 30000;
#define ISMATE (SCORE_INF - MAX_DEPTH)

// Default starting position (as a constant)
constexpr const char START_FEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// #define DEBUG
#ifndef DEBUG
#define ASSERT(n)
#else
#define ASSERT(n)                                                                                                                                    \
    if (!(n)) {                                                                                                                                      \
        printf("%s - Failed", #n);                                                                                                                   \
        printf("On %s ", __DATE__);                                                                                                                  \
        printf("At %s ", __TIME__);                                                                                                                  \
        printf("In File %s ", __FILE__);                                                                                                             \
        printf("At Line %d\n", __LINE__);                                                                                                            \
        exit(1);                                                                                                                                     \
    }
#endif

typedef unsigned long long U64;

enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK };

enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };

enum { WHITE, BLACK, BOTH };

enum {
    A1 = 21, B1, C1, D1, E1, F1, G1, H1,
    A2 = 31, B2, C2, D2, E2, F2, G2, H2,
    A3 = 41, B3, C3, D3, E3, F3, G3, H3,
    A4 = 51, B4, C4, D4, E4, F4, G4, H4,
    A5 = 61, B5, C5, D5, E5, F5, G5, H5,
    A6 = 71, B6, C6, D6, E6, F6, G6, H6,
    A7 = 81, B7, C7, D7, E7, F7, G7, H7,
    A8 = 91, B8, C8, D8, E8, F8, G8, H8,
    NO_SQ,
    OFFBOARD
};

enum { FALSE, TRUE };

enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };

typedef struct {
    int move;
    int score;
} S_MOVE;

// 0000 0000 0000 0000 0000 0111 1111 -> From (0x7F)
// 0000 0000 0000 0011 1111 1000 0000 -> To (>> 7,  0x7F)
// 0000 0000 0011 1100 0000 0000 0000 -> Captured (>> 14, 0xF)
// 0000 0000 0100 0000 0000 0000 0000 -> EnPas (0x40000)
// 0000 0000 1000 0000 0000 0000 0000 -> Pawn Start (0x80000)
// 0000 1111 0000 0000 0000 0000 0000 -> Promotion (>> 20, 0xF)
// 0001 0000 1000 0000 0000 0000 0000 -> Castled (0x1000000)

enum { HFNONE, HFALPHA, HFBETA, HFEXACT };

typedef struct {
    U64 posKey;
    int move;
    short score;
    unsigned char depth;
    unsigned char flags;
} S_HASHENTRY;

typedef struct{

} Search;

typedef struct {
    S_HASHENTRY *pTable;
    int numEntries; // power of two
    U64 mask;       // numEntries - 1
    int newWrite;
    int overWrite;
} S_HASHTABLE;

typedef struct {
    int move;
    int castlePerm;
    int enPas;
    int fiftyMove;
    U64 posKey;
} Undo;

typedef struct {
    S_MOVE moves[MAX_POSITION_MOVES];
    int count;
} MoveList;

typedef struct {
    int pieces[120];
    U64 pawns[3];

    int kingSq[2];
    int side;
    int enPas;
    int fiftyMove;

    int ply;
    int hisPly;

    int castlePerm;

    U64 posKey;
    int pceNum[13];
    int bigPce[3];
    int majPce[3];
    int minPce[3];

    int material[2];

    Undo history[MAX_GAME_MOVES];

    int plist[13][10];

    S_HASHTABLE HashTable[1];
    int PvArray[MAX_DEPTH];

    int searchHistory[13][BOARD_SIZE];
    int searchKillers[2][MAX_DEPTH];
} Board;

typedef struct {
    int startTime;
    int stopTime;
    int depth;
    int depthSet;
    int timeset;
    int movestogo;
    int infinite;
    long nodes;
    int quit;
    int stopped;

    float fh;
    float fhf;
} SearchInfo;

// Macros
#define FR2SQ(f, r) ((21 + (f)) + ((r)*10))
#define SQ64(sq120) Sq120ToSq64[sq120]
#define SQ120(sq64) Sq64ToSq120[sq64]
#define POP(b) PopBit(b)
#define CNT(b) CountBit(b)
#define CLRBIT(bb, sq) ((bb) &= ClearMask[(sq)])
#define SETBIT(bb, sq) ((bb) |= SetMask[(sq)])

#define IsBQ(p) (PieceBishopQueen[(p)])
#define IsRQ(p) (PieceRookQueen[(p)])
#define IsKi(p) (PieceKing[(p)])
#define IsKn(p) (PieceKnight[(p)])

#define FROMSQ(m) ((m)&0x7F)
#define TOSQ(m) (((m) >> 7) & 0x7F)
#define CAPTURED(m) (((m) >> 14) & 0xF)
#define PROMOTED(m) (((m) >> 20) & 0xF)
#define MFLAGEP 0x40000
#define MFLAGPS 0x80000
#define MFLAGCA 0x1000000

#define MFLAGCAP 0x7c000
#define MFLAGPROM 0xF00000

#define NOMOVE 0

// Globals
extern int Sq120ToSq64[BOARD_SIZE];
extern int Sq64ToSq120[64];
extern U64 SetMask[64];
extern U64 ClearMask[64];
extern U64 PieceKeys[13][120];
extern U64 SideKey;
extern U64 CastleKeys[16];

extern int FilesBrd[BOARD_SIZE];
extern int RanksBrd[BOARD_SIZE];

extern char PceChar[];
extern char SideChar[];
extern char RankChar[];
extern char FileChar[];

extern int PieceBig[13];
extern int PieceMaj[13];
extern int PieceMin[13];
extern int PieceVal[13];
extern int PieceCol[13];
extern int PiecePawn[13];

extern int PieceKnight[13];
extern int PieceKing[13];
extern int PieceRookQueen[13];
extern int PieceBishopQueen[13];
extern int PieceSlides[13];

extern int FilesBrd[BOARD_SIZE];
extern int RanksBrd[BOARD_SIZE];

extern int Mirror64[64];

extern U64 FileBBMask[8];
extern U64 RankBBMask[8];

extern U64 BlackPassedMask[64];
extern U64 WhitePassedMask[64];
extern U64 IsolatedMask[64];
extern U64 PawnShield[64];

// init.c
extern void AllInit();

// bitboards.c
extern void PrintBitBoard(U64 bb);
extern int PopBit(U64 *bb);
extern int CountBit(U64 b);

// hashkeys.c
extern U64 GeneratePosKey(const Board *pos);

// board.c
extern void UpdateListsMaterials(Board *pos);
extern void ResetBoard(Board *pos);
extern int ParseFen(const char *fen, Board *pos);
extern void PrintBoard(Board *pos);
extern int CheckBoard(Board *pos);

// attack.c
extern int SqAttacked(const int sq, const int side, Board *pos);

// io.c
extern char *PrMove(const int move);
extern char *PrSq(const int sq);
extern int ParseMove(const char *ptrChar, Board *pos);
extern void PrintMoveList(MoveList *list);
extern void PrintBin(int move);

// movegen.c
extern void GenerateAllMoves(Board *pos, MoveList *list);
extern void GenerateAllCaptures(Board *pos, MoveList *list);
extern int MoveExists(Board *pos, const int move);
extern int InitMvvLva();
extern void TakeNullMove(Board *pos);
extern void MakeNullMove(Board *pos);

// validate.c
extern int SqOnBoard(const int sq);
extern int SideValid(const int side);
extern int FileRankValid(const int fr);
extern int PieceValidEmpty(const int pce);
extern int PieceValid(const int pce);

// makemove.c
extern int MakeMove(Board *pos, int move);
extern void TakeMove(Board *pos);

// perft.c
extern void PerftTest(int depth, Board *pos);

// pvtable.c
extern void StoreHashEntry(Board *pos, const int move, int score, const int flags, const int depth);
extern int ProbeHashEntry(Board *pos, int *move, int *score, int alpha, int beta, int depth);
extern void InitHashTable(S_HASHTABLE *table);
extern void ClearHashTable(S_HASHTABLE *table);
extern int GetPvLine(const int depth, Board *pos);
extern int ProbePvMove(const Board *pos);

// evaluate.c
extern int EvaluatePosition(Board *pos);

// search.c
extern int IsRepetition(const Board *pos);
extern void SearchPosition(Board *pos, SearchInfo *info);

// misc.c
extern int GetTimeMS();
extern void ReadInput(SearchInfo *info);

// uci.c
extern void Uci_Loop();
