#include <stdio.h>
#include "types.hpp"

#define MOVE(f, t, ca, pro, fl) ((f) | ((t) << 7) | ((ca) << 14) | ((pro) << 20) | (fl))
#define SQOFFBOARD(sq) (FilesBrd[(sq)] == OFFBOARD)

static int LoopSlidePiece[8] = { wB, wR, wQ, 0, bB, bR, bQ, 0 };
static int LoopNonSlidePiece[6] = { wN, wK, 0, bN, bK, 0 };

static int LoopSlideIndex[2] = { 0, 4 };
static int LoopNonSlideIndex[2] = { 0, 3 };

static int PceDir[13][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { -8, -19, -21, -12, 8, 19, 21, 12 },
    { -9, -11, 11, 9, 0, 0, 0, 0 },
    { -1, -10, 1, 10, 0, 0, 0, 0 },
    { -1, -10, 1, 10, -9, -11, 11, 9 },
    { -1, -10, 1, 10, -9, -11, 11, 9 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { -8, -19, -21, -12, 8, 19, 21, 12 },
    { -9, -11, 11, 9, 0, 0, 0, 0 },
    { -1, -10, 1, 10, 0, 0, 0, 0 },
    { -1, -10, 1, 10, -9, -11, 11, 9 },
    { -1, -10, 1, 10, -9, -11, 11, 9 }
};

int NumDir[13] = { 0, 0, 8, 4, 4, 8, 8, 0, 8, 4, 4, 8, 8 };

const int VictimScore[13] = { 0, 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600 };
static int MvvLvaScores[13][13];

int InitMvvLva() {
    int Attacker;
    int Victim;
    for(Attacker = wP; Attacker <= bK; ++Attacker) {
        for(Victim = wP; Victim <= bK; ++Victim) {
            MvvLvaScores[Victim][Attacker] = VictimScore[Victim] + 6 - (VictimScore[Attacker] / 100);
        }
    }
    return 0;
}

static void AddQuietMove(Board *pos, int move, MoveList *list) {
    list->moves[list->count].move = move;
    if(pos->searchKillers[0][pos->ply] == move) {
        list->moves[list->count].score = 900000;
    }
    else if(pos->searchKillers[1][pos->ply] == move) {
        list->moves[list->count].score = 800000;
    }
    else {
        list->moves[list->count].score = pos->searchHistory[pos->pieces[FROMSQ(move)]][TOSQ(move)];
    }
    list->count++;
}

static void AddCaptureMove(Board *pos, int move, MoveList *list) {
    list->moves[list->count].move = move;
    list->moves[list->count].score = MvvLvaScores[CAPTURED(move)][pos->pieces[FROMSQ(move)]] + 1000000;
    if(PROMOTED(move)) {
        switch(PROMOTED(move)) {
            case wQ: case bQ: list->moves[list->count].score += 900000; break;
            case wR: case bR: list->moves[list->count].score += 500000; break;
            case wB: case bB: list->moves[list->count].score += 300000; break;
            case wN: case bN: list->moves[list->count].score += 100000; break;
        }
    }
    list->count++;
}

static void AddEnPassantMove(Board *pos, int move, MoveList *list) {
    list->moves[list->count].move = move;
    list->moves[list->count].score = 105 + 1000000;
    list->count++;
}

static void AddWhitePawnMove(Board *pos, const int from, const int to, const int cap, MoveList *list) {
    if(RanksBrd[from] == RANK_7) {
        AddCaptureMove(pos, MOVE(from, to, EMPTY, wQ, 0), list);
        AddCaptureMove(pos, MOVE(from, to, EMPTY, wR, 0), list);
        AddCaptureMove(pos, MOVE(from, to, EMPTY, wB, 0), list);
        AddCaptureMove(pos, MOVE(from, to, EMPTY, wN, 0), list);
    } else {
        AddCaptureMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
    }
}

static void AddWhitePawnCapture(Board *pos, const int from, const int to, const int cap, MoveList *list) {
    if(RanksBrd[from] == RANK_7) {
        AddCaptureMove(pos, MOVE(from, to, cap, wQ, 0), list);
        AddCaptureMove(pos, MOVE(from, to, cap, wR, 0), list);
        AddCaptureMove(pos, MOVE(from, to, cap, wB, 0), list);
        AddCaptureMove(pos, MOVE(from, to, cap, wN, 0), list);
    } else {
        AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
    }
}

static void AddBlackPawnMove(Board *pos, const int from, const int to, const int cap, MoveList *list) {
    if(RanksBrd[from] == RANK_2) {
        AddCaptureMove(pos, MOVE(from, to, EMPTY, bQ, 0), list);
        AddCaptureMove(pos, MOVE(from, to, EMPTY, bR, 0), list);
        AddCaptureMove(pos, MOVE(from, to, EMPTY, bB, 0), list);
        AddCaptureMove(pos, MOVE(from, to, EMPTY, bN, 0), list);
    } else {
        AddCaptureMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
    }
}

static void AddBlackPawnCapture(Board *pos, const int from, const int to, const int cap, MoveList *list) {
    if(RanksBrd[from] == RANK_2) {
        AddCaptureMove(pos, MOVE(from, to, cap, bQ, 0), list);
        AddCaptureMove(pos, MOVE(from, to, cap, bR, 0), list);
        AddCaptureMove(pos, MOVE(from, to, cap, bB, 0), list);
        AddCaptureMove(pos, MOVE(from, to, cap, bN, 0), list);
    } else {
        AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
    }
}

void GenerateAllMoves(Board *pos, MoveList *list) {
    ASSERT(CheckBoard(pos));

    list->count = 0;

    int pce = EMPTY;
    int side = pos->side;
    int sq = 0; int t_sq = 0;
    int pceNum = 0; 
    int pceIndex = 0;
    int index = 0;

    if(side == WHITE) {
        for(pceNum = 0; pceNum<pos->pceNum[wP]; ++pceNum) {
            sq = pos->plist[wP][pceNum];
            ASSERT(SqOnBoard(sq));

            if(pos->pieces[sq + 10] == EMPTY) {
                AddWhitePawnMove(pos, sq, sq+10, EMPTY, list);
                if(RanksBrd[sq] == RANK_2 && pos->pieces[sq + 20] == EMPTY) {
                    AddQuietMove(pos, MOVE(sq, (sq+20), EMPTY, EMPTY, MFLAGPS), list);
                }
            }

            if(!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
                AddWhitePawnCapture(pos, sq, sq+9, pos->pieces[sq + 9], list);
            }

            if(!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
                AddWhitePawnCapture(pos, sq, sq+11, pos->pieces[sq + 11], list);
            }
            if(pos->enPas != NO_SQ) {
                if(pos->enPas == (sq + 9)) {
                    AddEnPassantMove(pos, MOVE(sq, sq+9, EMPTY, EMPTY, MFLAGEP), list);
                }
                if(pos->enPas == (sq + 11)) {
                    AddEnPassantMove(pos, MOVE(sq, sq+11, EMPTY, EMPTY, MFLAGEP), list);
                }
            }
        }
        if(pos->castlePerm & WKCA) {
            if(pos->pieces[F1] == EMPTY && pos->pieces[G1] == EMPTY) {
                if(!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos)) {
                    AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MFLAGCA), list);
                }
            }
        }

        if(pos->castlePerm & WQCA) {
            if(pos->pieces[D1] == EMPTY && pos->pieces[C1] == EMPTY && pos->pieces[B1] == EMPTY) {
                if(!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos) && !SqAttacked(C1, BLACK, pos)) {
                    AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MFLAGCA), list);
                }
            }
        }
    } else {
        for(pceNum = 0; pceNum<pos->pceNum[bP]; ++pceNum) {
            sq = pos->plist[bP][pceNum];
            ASSERT(SqOnBoard(sq));

            if(pos->pieces[sq - 10] == EMPTY) {
                AddBlackPawnMove(pos, sq, sq-10, EMPTY, list);
                if(RanksBrd[sq] == RANK_7 && pos->pieces[sq - 20] == EMPTY) {
                    AddQuietMove(pos, MOVE(sq, (sq-20), EMPTY, EMPTY, MFLAGPS), list);
                }
            }

            if(!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
                AddBlackPawnCapture(pos, sq, sq-9, pos->pieces[sq - 9], list);
            }

            if(!SQOFFBOARD(sq -11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
                AddBlackPawnCapture(pos, sq, sq-11, pos->pieces[sq - 11], list);
            }
            if(pos->enPas != NO_SQ) {
                if(pos->enPas == (sq - 9)) {
                    AddEnPassantMove(pos, MOVE(sq, sq-9, EMPTY, EMPTY, MFLAGEP), list);
                }
                if(pos->enPas == (sq - 11)) {
                    AddEnPassantMove(pos, MOVE(sq, sq-11, EMPTY, EMPTY, MFLAGEP), list);
                }
            }
        }

        if(pos->castlePerm & BKCA) {
            if(pos->pieces[F8] == EMPTY && pos->pieces[G8] == EMPTY) {
                if(!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos) && !SqAttacked(G8, WHITE, pos)) {
                    AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MFLAGCA), list);
                }
            }
        }

        if(pos->castlePerm & BQCA) {
            if(pos->pieces[D8] == EMPTY && pos->pieces[C8] == EMPTY && pos->pieces[B8] == EMPTY) {
                if(!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos) && !SqAttacked(C8, WHITE, pos)) {
                    AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MFLAGCA), list);
                }
            }
        }
    }

    int dir = 0;

    //Sliding pieces
    pceIndex = LoopSlideIndex[side];
    pce = LoopSlidePiece[pceIndex++];

    while(pce != 0) {
        ASSERT(PieceValid(pce));
        for(pceNum = 0; pceNum<pos->pceNum[pce]; ++pceNum) {
            sq = pos->plist[pce][pceNum];
            ASSERT(SqOnBoard(sq));
            for(index = 0; index < NumDir[pce]; ++index) {
                dir = PceDir[pce][index];
                t_sq = sq + dir;
                while(!SQOFFBOARD(t_sq)) {
                    if(pos->pieces[t_sq] != EMPTY) {
                        if(PieceCol[pos->pieces[t_sq]] != side) {
                            AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
                        }
                        break;
                    }
                    AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
                    t_sq += dir;
                }
            }
        }

        pce = LoopSlidePiece[pceIndex++];
    }

    //Non-sliding pieces;

    pceIndex = LoopNonSlideIndex[side];
    pce = LoopNonSlidePiece[pceIndex++];
    
    while(pce != 0) {
        ASSERT(PieceValid(pce));
        for(pceNum = 0; pceNum<pos->pceNum[pce]; ++pceNum) {
            sq = pos->plist[pce][pceNum];
            ASSERT(SqOnBoard(sq));
            for(index = 0; index < NumDir[pce]; ++index) {
                dir = PceDir[pce][index];
                t_sq = sq + dir;
                if(SQOFFBOARD(t_sq)) {
                    continue;
                }
                if(pos->pieces[t_sq] != EMPTY) {
                    if(PieceCol[pos->pieces[t_sq]] != side) {
                        AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
                    }
                    continue;
                }
                AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
            }
        }

        pce = LoopNonSlidePiece[pceIndex++];
    }
}

void GenerateAllCaptures(Board *pos, MoveList *list) {
    ASSERT(CheckBoard(pos));

    list->count = 0;

    int pce = EMPTY;
    int side = pos->side;
    int sq = 0; int t_sq = 0;
    int pceNum = 0; 
    int pceIndex = 0;
    int index = 0;

    if(side == WHITE) {
        for(pceNum = 0; pceNum<pos->pceNum[wP]; ++pceNum) {
            sq = pos->plist[wP][pceNum];
            ASSERT(SqOnBoard(sq));

            if(!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
                AddWhitePawnCapture(pos, sq, sq+9, pos->pieces[sq + 9], list);
            }

            if(!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
                AddWhitePawnCapture(pos, sq, sq+11, pos->pieces[sq + 11], list);
            }
            if(pos->enPas != NO_SQ) {
                if(pos->enPas == (sq + 9)) {
                    AddEnPassantMove(pos, MOVE(sq, sq+9, EMPTY, EMPTY, MFLAGEP), list);
                }
                if(pos->enPas == (sq + 11)) {
                    AddEnPassantMove(pos, MOVE(sq, sq+11, EMPTY, EMPTY, MFLAGEP), list);
                }
            }
        }
    } else {
        for(pceNum = 0; pceNum<pos->pceNum[bP]; ++pceNum) {
            sq = pos->plist[bP][pceNum];
            ASSERT(SqOnBoard(sq));

            if(!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
                AddBlackPawnCapture(pos, sq, sq-9, pos->pieces[sq - 9], list);
            }

            if(!SQOFFBOARD(sq -11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
                AddBlackPawnCapture(pos, sq, sq-11, pos->pieces[sq - 11], list);
            }
            if(pos->enPas != NO_SQ) {
                if(pos->enPas == (sq - 9)) {
                    AddEnPassantMove(pos, MOVE(sq, sq-9, EMPTY, EMPTY, MFLAGEP), list);
                }
                if(pos->enPas == (sq - 11)) {
                    AddEnPassantMove(pos, MOVE(sq, sq-11, EMPTY, EMPTY, MFLAGEP), list);
                }
            }
        }
    }

    int dir = 0;

    //Sliding pieces
    pceIndex = LoopSlideIndex[side];
    pce = LoopSlidePiece[pceIndex++];

    while(pce != 0) {
        ASSERT(PieceValid(pce));
        for(pceNum = 0; pceNum<pos->pceNum[pce]; ++pceNum) {
            sq = pos->plist[pce][pceNum];
            ASSERT(SqOnBoard(sq));
            for(index = 0; index < NumDir[pce]; ++index) {
                dir = PceDir[pce][index];
                t_sq = sq + dir;
                while(!SQOFFBOARD(t_sq)) {
                    if(pos->pieces[t_sq] != EMPTY) {
                        if(PieceCol[pos->pieces[t_sq]] != side) {
                            AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
                        }
                        break;
                    }
                    t_sq += dir;
                }
            }
        }

        pce = LoopSlidePiece[pceIndex++];
    }

    //Non-sliding pieces;

    pceIndex = LoopNonSlideIndex[side];
    pce = LoopNonSlidePiece[pceIndex++];
    
    while(pce != 0) {
        ASSERT(PieceValid(pce));
        for(pceNum = 0; pceNum<pos->pceNum[pce]; ++pceNum) {
            sq = pos->plist[pce][pceNum];
            ASSERT(SqOnBoard(sq));
            for(index = 0; index < NumDir[pce]; ++index) {
                dir = PceDir[pce][index];
                t_sq = sq + dir;
                if(SQOFFBOARD(t_sq)) {
                    continue;
                }
                if(pos->pieces[t_sq] != EMPTY) {
                    if(PieceCol[pos->pieces[t_sq]] != side) {
                        AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
                    }
                    continue;
                }
            }
        }

        pce = LoopNonSlidePiece[pceIndex++];
    }
}

int MoveExists(Board *pos, const int move) {
    MoveList list[1];
    GenerateAllMoves(pos, list);
    int MoveNum = 0;
    for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {
        if(!MakeMove(pos, list->moves[MoveNum].move)) {
            continue;
        }
        TakeMove(pos);
        if(list->moves[MoveNum].move == move) {
            return TRUE;
        }
    }
    return FALSE;
}
