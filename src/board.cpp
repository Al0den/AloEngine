#include <stdio.h>
#include "types.hpp"

int CheckBoard(Board *pos) {
    int t_pceNum[13] = {0};
    int t_bigPce[2] = {0};
    int t_majPce[2] = {0};
    int t_minPce[2] = {0};
    int t_material[2] = {0};

    int sq64, t_piece, t_pce_num, sq120, colour, pcount;

    U64 t_pawns[3] = {0ULL};

    t_pawns[WHITE] = pos->pawns[WHITE];
    t_pawns[BLACK] = pos->pawns[BLACK];
    t_pawns[BOTH] = pos->pawns[BOTH];

    for(t_piece = wP; t_piece <= bK; ++t_piece) {
        for(t_pce_num = 0; t_pce_num < pos->pceNum[t_piece]; ++t_pce_num) {
            sq120 = pos->plist[t_piece][t_pce_num];
            ASSERT(pos->pieces[sq120] == t_piece);
        }
    }

    for(sq64 = 0; sq64 < 64; ++sq64) {
        sq120 = SQ120(sq64);
        t_piece = pos->pieces[sq120];
        t_pceNum[t_piece]++;
        colour = PieceCol[t_piece];
        if(PieceBig[t_piece] == TRUE) t_bigPce[colour]++;
        if(PieceMin[t_piece] == TRUE) t_minPce[colour]++;
        if(PieceMaj[t_piece] == TRUE) t_majPce[colour]++;

        t_material[colour] += PieceVal[t_piece];
    }

    for(t_piece = wP; t_piece <= bK; ++t_piece) {
        ASSERT(t_pceNum[t_piece] == pos->pceNum[t_piece]);
    }

    pcount = CNT(t_pawns[WHITE]);
    ASSERT(pcount == pos->pceNum[wP]);
    pcount = CNT(t_pawns[BLACK]);
    ASSERT(pcount == pos->pceNum[bP]);
    pcount = CNT(t_pawns[BOTH]);
    ASSERT(pcount == (pos->pceNum[bP] + pos->pceNum[wP]));

    while(t_pawns[WHITE]) {
        sq64 = POP(&t_pawns[WHITE]);
        ASSERT(pos->pieces[SQ120(sq64)] == wP);
    }

    while(t_pawns[BLACK]) {
        sq64 = POP(&t_pawns[BLACK]);
        ASSERT(pos->pieces[SQ120(sq64)] == bP);
    }

    while(t_pawns[BOTH]) {
        sq64 = POP(&t_pawns[BOTH]);
        ASSERT((pos->pieces[SQ120(sq64)] == bP) || (pos->pieces[SQ120(sq64)] == wP));
    }

    ASSERT(t_material[WHITE] == pos->material[WHITE] && t_material[BLACK] == pos->material[BLACK]);
    ASSERT(t_minPce[WHITE] == pos->minPce[WHITE] && t_minPce[BLACK] == pos->minPce[BLACK]);
    ASSERT(t_majPce[WHITE] == pos->majPce[WHITE] && t_majPce[BLACK] == pos->majPce[BLACK]);
    ASSERT(t_bigPce[WHITE] == pos->bigPce[WHITE] && t_bigPce[BLACK] == pos->bigPce[BLACK]);

    ASSERT(pos->side == WHITE || pos->side == BLACK);
    ASSERT(GeneratePosKey(pos) == pos->posKey);

    ASSERT(pos->enPas == NO_SQ || (RanksBrd[pos->enPas] == RANK_6 && pos->side == WHITE) || (RanksBrd[pos->enPas] == RANK_3 && pos->side == BLACK));
    ASSERT(pos->pieces[pos->kingSq[WHITE]] == wK);
    ASSERT(pos->pieces[pos->kingSq[BLACK]] == bK);
    return 1;
}

void UpdateListsMaterials(Board *pos) {
    int piece = EMPTY;
    int sq = 0;
    int index = 0;
    int colour = 0;
    for(index = 0; index < BOARD_SIZE; ++index) {
        sq = index;
        piece = pos->pieces[index];

        if(piece != OFFBOARD && piece != EMPTY) {
            colour = PieceCol[piece];
            if(PieceBig[piece] == TRUE) pos->bigPce[colour]++;
            if(PieceMin[piece] == TRUE) pos->minPce[colour]++;
            if(PieceMaj[piece] == TRUE) pos->majPce[colour]++;

            pos->material[colour] += PieceVal[piece];

            pos->plist [piece][pos->pceNum[piece]] = sq;
            pos->pceNum[piece]++;

            if(piece == wK) pos->kingSq[WHITE] = sq;
            if(piece == bK) pos->kingSq[BLACK] = sq;

            if(piece == wP) {
                SETBIT(pos->pawns[WHITE], SQ64(sq));
                SETBIT(pos->pawns[BOTH], SQ64(sq));
            } else if(piece == bP) {
                SETBIT(pos->pawns[BLACK], SQ64(sq));
                SETBIT(pos->pawns[BOTH], SQ64(sq));
            }
        }
        
    }
}

int ParseFen(const char *fen, Board *pos) {
    ASSERT(fen != NULL);
    ASSERT(pos != NULL);

    int rank = RANK_8;
    int file = FILE_A;
    int piece = 0;
    int count = 0;
    int i =0;
    int sq64 = 0;
    int sq120 = 0;

    ResetBoard(pos);

    while(rank >= RANK_1 && *fen) {
        count = 1;
        switch (*fen) {
            case 'p': piece = bP; break;
            case 'r': piece = bR; break;
            case 'n': piece = bN; break;
            case 'b': piece = bB; break;
            case 'k': piece = bK; break;
            case 'q': piece = bQ; break;
            case 'P': piece = wP; break;
            case 'R': piece = wR; break;
            case 'N': piece = wN; break;
            case 'B': piece = wB; break;
            case 'K': piece = wK; break;
            case 'Q': piece = wQ; break;
            case '1': piece = EMPTY; break;
            case '2': piece = EMPTY; count = 2; break;
            case '3': piece = EMPTY; count = 3; break;
            case '4': piece = EMPTY; count = 4; break;
            case '5': piece = EMPTY; count = 5; break;
            case '6': piece = EMPTY; count = 6; break;
            case '7': piece = EMPTY; count = 7; break;
            case '8': piece = EMPTY; count = 8; break;
            case '/':
            case ' ':
                rank--;
                file = FILE_A;
                fen++;
                continue;
            default:
                printf("FEN error \n");
                return -1;
        }
        for(i = 0; i < count; ++i) {
            sq64 = rank * 8 + file;
            sq120 = SQ120(sq64);
            if(piece != EMPTY) {
                pos->pieces[sq120] = piece;
            }
            file++;
        }
        fen++;
    }
    ASSERT(*fen == 'w' || *fen == 'b');
    pos->side = (*fen == 'w') ? WHITE : BLACK;
    fen += 2;
    #ifdef DEBUG
    printf("%s\n", fen);
    #endif
    for(i = 0; i < 4; i++) {
        if(*fen == ' ') {
            break;
        }
        switch(*fen) {
            case 'K': pos->castlePerm |= WKCA; break;
            case 'Q': pos->castlePerm |= WQCA; break;
            case 'k': pos->castlePerm |= BKCA; break;
            case 'q': pos->castlePerm |= BQCA; break;
            default: break;
        }
        fen++;
    }
    fen++;

    ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);

    if(*fen != '-') {
        file = fen[0] - 'a';
        rank = fen[1] - '1';
        ASSERT(file >= FILE_A && file <= FILE_H);
        ASSERT(rank >= RANK_1 && rank <= RANK_8);
        pos->enPas = FR2SQ(file, rank);
    }

    pos->posKey = GeneratePosKey(pos);

    UpdateListsMaterials(pos);

    return 0;
}

void ResetBoard(Board *pos) {
    int index = 0;

    for(index = 0 ; index < BOARD_SIZE ; ++index) {
        pos->pieces[index] = OFFBOARD;
    }

    for(index = 0; index < 64; ++index) {
        pos->pieces[SQ120(index)] = EMPTY;
    }

    for(index = 0; index < 2; ++index) {
        pos->bigPce[index] = 0;
        pos->majPce[index] = 0;
        pos->minPce[index] = 0;
        pos->material[index] = 0;
        pos->pawns[index] = 0ULL;
        pos->material[index] = 0;
    }

    for(index = 0; index < 3; ++index) {
        pos->pawns[index] = 0ULL;
    }

    for(index = 0; index < 13; ++index) {
        pos->pceNum[index] = 0;
    }

    pos->kingSq[WHITE] = pos->kingSq[BLACK] = NO_SQ;

    pos->side = BOTH;
    pos->enPas = NO_SQ;
    pos->fiftyMove = 0;
    
    pos->ply = 0;
    pos->hisPly = 0;

    pos->castlePerm = 0;
    pos->posKey = 0ULL;


}

void PrintBoard(Board *pos) {
    int sq, file, rank, piece;
    printf("\nGame board:\n");

    for(rank = RANK_8; rank >= RANK_1; --rank) {
        printf("%d  ", rank + 1);
        for(file = FILE_A; file <= FILE_H; ++file) {
            sq = FR2SQ(file, rank);
            piece = pos->pieces[sq];
            printf("%3c", PceChar[piece]);
        }
        printf("\n");
    }
    printf("\n   ");
    for(file = FILE_A; file <= FILE_H; ++file) {
        printf("%3c", 'a' + file);
    }
    printf("\n");

    printf("\nSide: %c\n", SideChar[pos->side]);
    printf("En passant: %d\n", pos->enPas);
    printf("Castle: %c%c%c%c\n",
            pos->castlePerm & WKCA ? 'K' : '-',
            pos->castlePerm & WQCA ? 'Q' : '-',
            pos->castlePerm & BKCA ? 'k' : '-',
            pos->castlePerm & BQCA ? 'q' : '-');
    printf("PosKey: %llX\n", pos->posKey);
}

void MirrorBoard(Board *pos) {
    int tempPiecesArray[64];
    int tempSide = pos->side ^ 1;
    int SwapPiece[13] = {EMPTY, bP, bN, bB, bR, bQ, bK, wP, wN, wB, wR, wQ, wK};
    int tempCastlePerm = 0;
    int tempEnPas = NO_SQ;

    int sq;
    int tp;

    if(pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
    if(pos->castlePerm & WQCA) tempCastlePerm |= BQCA;
    if(pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
    if(pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

    if(pos->enPas != NO_SQ) {
        tempEnPas = SQ120(Mirror64[SQ64(pos->enPas)]);
    }

    for(sq = 0; sq < 64; ++sq) {
        tempPiecesArray[sq] = pos->pieces[SQ120(Mirror64[sq])];
    }

    ResetBoard(pos);

    for(sq = 0; sq < 64; ++sq) {
        tp = SwapPiece[tempPiecesArray[sq]];
        pos->pieces[SQ120(sq)] = tp;
    }

    pos->side = tempSide;
    pos->castlePerm = tempCastlePerm;
    pos->enPas = tempEnPas;

    pos->posKey = GeneratePosKey(pos);

    UpdateListsMaterials(pos);
    ASSERT(CheckBoard(pos));
}
