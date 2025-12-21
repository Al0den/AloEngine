#include <stdio.h>
#include "alo/types.hpp"

int Board::check() const {
    int t_pceNum[13] = {0};
    int t_bigPce[2] = {0};
    int t_majPce[2] = {0};
    int t_minPce[2] = {0};
    int t_material[2] = {0};

    int sq64, t_piece, t_pce_num, sq120, colour, pcount;

    U64 t_pawns[3] = {0ULL};

    t_pawns[WHITE] = pawns[WHITE];
    t_pawns[BLACK] = pawns[BLACK];
    t_pawns[BOTH] = pawns[BOTH];

    for(t_piece = wP; t_piece <= bK; ++t_piece) {
        for(t_pce_num = 0; t_pce_num < pceNum[t_piece]; ++t_pce_num) {
            sq120 = plist[t_piece][t_pce_num];
            ASSERT(pieces[sq120] == t_piece);
        }
    }

    for(sq64 = 0; sq64 < 64; ++sq64) {
        sq120 = SQ120(sq64);
        t_piece = pieces[sq120];
        t_pceNum[t_piece]++;
        colour = PieceCol[t_piece];
        if(PieceBig[t_piece] == TRUE) t_bigPce[colour]++;
        if(PieceMin[t_piece] == TRUE) t_minPce[colour]++;
        if(PieceMaj[t_piece] == TRUE) t_majPce[colour]++;

        t_material[colour] += PieceVal[t_piece];
    }

    for(t_piece = wP; t_piece <= bK; ++t_piece) {
        ASSERT(t_pceNum[t_piece] == pceNum[t_piece]);
    }

    pcount = CNT(t_pawns[WHITE]);
    ASSERT(pcount == pceNum[wP]);
    pcount = CNT(t_pawns[BLACK]);
    ASSERT(pcount == pceNum[bP]);
    pcount = CNT(t_pawns[BOTH]);
    ASSERT(pcount == (pceNum[bP] + pceNum[wP]));

    while(t_pawns[WHITE]) {
        sq64 = POP(&t_pawns[WHITE]);
        ASSERT(pieces[SQ120(sq64)] == wP);
    }

    while(t_pawns[BLACK]) {
        sq64 = POP(&t_pawns[BLACK]);
        ASSERT(pieces[SQ120(sq64)] == bP);
    }

    while(t_pawns[BOTH]) {
        sq64 = POP(&t_pawns[BOTH]);
        ASSERT((pieces[SQ120(sq64)] == bP) || (pieces[SQ120(sq64)] == wP));
    }

    ASSERT(t_material[WHITE] == material[WHITE] && t_material[BLACK] == material[BLACK]);
    ASSERT(t_minPce[WHITE] == minPce[WHITE] && t_minPce[BLACK] == minPce[BLACK]);
    ASSERT(t_majPce[WHITE] == majPce[WHITE] && t_majPce[BLACK] == majPce[BLACK]);
    ASSERT(t_bigPce[WHITE] == bigPce[WHITE] && t_bigPce[BLACK] == bigPce[BLACK]);

    ASSERT(side == WHITE || side == BLACK);
    ASSERT(GeneratePosKey(this) == posKey);

    ASSERT(enPas == NO_SQ || (RanksBrd[enPas] == RANK_6 && side == WHITE) || (RanksBrd[enPas] == RANK_3 && side == BLACK));
    ASSERT(pieces[kingSq[WHITE]] == wK);
    ASSERT(pieces[kingSq[BLACK]] == bK);
    return 1;
}

void Board::updateListsMaterials() {
    int piece = EMPTY;
    int sq = 0;
    int index = 0;
    int colour = 0;
    for(index = 0; index < BOARD_SIZE; ++index) {
        sq = index;
        piece = pieces[index];

        if(piece != OFFBOARD && piece != EMPTY) {
            colour = PieceCol[piece];
            if(PieceBig[piece] == TRUE) bigPce[colour]++;
            if(PieceMin[piece] == TRUE) minPce[colour]++;
            if(PieceMaj[piece] == TRUE) majPce[colour]++;

            material[colour] += PieceVal[piece];

            plist [piece][pceNum[piece]] = sq;
            pceNum[piece]++;

            if(piece == wK) kingSq[WHITE] = sq;
            if(piece == bK) kingSq[BLACK] = sq;

            if(piece == wP) {
                SETBIT(pawns[WHITE], SQ64(sq));
                SETBIT(pawns[BOTH], SQ64(sq));
            } else if(piece == bP) {
                SETBIT(pawns[BLACK], SQ64(sq));
                SETBIT(pawns[BOTH], SQ64(sq));
            }
        }
        
    }
}

int Board::parseFEN(const char *fen) {
    ASSERT(fen != NULL);

    int rank = RANK_8;
    int file = FILE_A;
    int piece = 0;
    int count = 0;
    int i =0;
    int sq64 = 0;
    int sq120 = 0;

    reset();

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
                pieces[sq120] = piece;
            }
            file++;
        }
        fen++;
    }
    ASSERT(*fen == 'w' || *fen == 'b');
    side = (*fen == 'w') ? WHITE : BLACK;
    fen += 2;
    #ifdef DEBUG
    printf("%s\n", fen);
    #endif
    for(i = 0; i < 4; i++) {
        if(*fen == ' ') {
            break;
        }
        switch(*fen) {
            case 'K': castlePerm |= WKCA; break;
            case 'Q': castlePerm |= WQCA; break;
            case 'k': castlePerm |= BKCA; break;
            case 'q': castlePerm |= BQCA; break;
            default: break;
        }
        fen++;
    }
    fen++;

    ASSERT(castlePerm >= 0 && castlePerm <= 15);

    if(*fen != '-') {
        file = fen[0] - 'a';
        rank = fen[1] - '1';
        ASSERT(file >= FILE_A && file <= FILE_H);
        ASSERT(rank >= RANK_1 && rank <= RANK_8);
        enPas = FR2SQ(file, rank);
    }

    posKey = GeneratePosKey(this);

    updateListsMaterials();

    return 0;
}



void Board::reset() {
    int index = 0;

    for(index = 0 ; index < BOARD_SIZE ; ++index) {
        pieces[index] = OFFBOARD;
    }

    for(index = 0; index < 64; ++index) {
        pieces[SQ120(index)] = EMPTY;
    }

    for(index = 0; index < 2; ++index) {
        bigPce[index] = 0;
        majPce[index] = 0;
        minPce[index] = 0;
        material[index] = 0;
        pawns[index] = 0ULL;
        material[index] = 0;
    }

    for(index = 0; index < 3; ++index) {
        pawns[index] = 0ULL;
    }

    for(index = 0; index < 13; ++index) {
        pceNum[index] = 0;
    }

    kingSq[WHITE] = kingSq[BLACK] = NO_SQ;

    side = BOTH;
    enPas = NO_SQ;
    fiftyMove = 0;
    
    ply = 0;
    hisPly = 0;

    castlePerm = 0;
    posKey = 0ULL;


}

int Board::toFEN(char *buf, size_t bufSize) const {
    if (!buf || bufSize < 64) return -1;

    int sq, file, rank, count;
    char *p = buf;
    char *end = buf + bufSize - 1;

    for (rank = RANK_8; rank >= RANK_1; --rank) {
        count = 0;
        for (file = FILE_A; file <= FILE_H; ++file) {
            sq = FR2SQ(file, rank);
            int piece = pieces[sq];

            if (piece == EMPTY) {
                ++count;
            } else {
                if (count) {
                    if (p >= end) return -1;
                    *p++ = char('0' + count);
                    count = 0;
                }
                if (p >= end) return -1;
                *p++ = PceChar[piece];
            }
        }
        if (count) {
            if (p >= end) return -1;
            *p++ = char('0' + count);
        }
        if (rank > RANK_1) {
            if (p >= end) return -1;
            *p++ = '/';
        }
    }

    if (p >= end) return -1;
    *p++ = ' ';
    *p++ = (side == WHITE ? 'w' : 'b');
    *p++ = ' ';

    int anyCastle = 0;
    if (castlePerm & WKCA) { *p++ = 'K'; anyCastle = 1; }
    if (castlePerm & WQCA) { *p++ = 'Q'; anyCastle = 1; }
    if (castlePerm & BKCA) { *p++ = 'k'; anyCastle = 1; }
    if (castlePerm & BQCA) { *p++ = 'q'; anyCastle = 1; }
    if (!anyCastle) { *p++ = '-'; }
    *p++ = ' ';

    if (enPas == NO_SQ) {
        *p++ = '-';
    } else {
        int fileEP = FilesBrd[enPas];
        int rankEP = RanksBrd[enPas];
        *p++ = char('a' + fileEP);
        *p++ = char('1' + rankEP);
    }

    *p++ = ' ';
    *p++ = '0'; 
    *p++ = ' ';
    *p++ = '1'; 

    *p = '\0';
    return 0;
}


void Board::print() const {
    int sq, file, rank, piece;
    printf("\nGame board:\n");

    for(rank = RANK_8; rank >= RANK_1; --rank) {
        printf("%d  ", rank + 1);
        for(file = FILE_A; file <= FILE_H; ++file) {
            sq = FR2SQ(file, rank);
            piece = pieces[sq];
            printf("%3c", PceChar[piece]);
        }
        printf("\n");
    }
    printf("\n   ");
    for(file = FILE_A; file <= FILE_H; ++file) {
        printf("%3c", 'a' + file);
    }
    printf("\n");

    printf("\nSide: %c\n", SideChar[side]);
    printf("En passant: %d\n", enPas);
    printf("Castle: %c%c%c%c\n",
            castlePerm & WKCA ? 'K' : '-',
            castlePerm & WQCA ? 'Q' : '-',
            castlePerm & BKCA ? 'k' : '-',
            castlePerm & BQCA ? 'q' : '-');
    printf("PosKey: %llX\n", posKey);
}

void Board::mirror() {
    int tempPiecesArray[64];
    int tempSide = side ^ 1;
    int SwapPiece[13] = {EMPTY, bP, bN, bB, bR, bQ, bK, wP, wN, wB, wR, wQ, wK};
    int tempCastlePerm = 0;
    int tempEnPas = NO_SQ;

    int sq;
    int tp;

    if(castlePerm & WKCA) tempCastlePerm |= BKCA;
    if(castlePerm & WQCA) tempCastlePerm |= BQCA;
    if(castlePerm & BKCA) tempCastlePerm |= WKCA;
    if(castlePerm & BQCA) tempCastlePerm |= WQCA;

    if(enPas != NO_SQ) {
        tempEnPas = SQ120(Mirror64[SQ64(enPas)]);
    }

    for(sq = 0; sq < 64; ++sq) {
        tempPiecesArray[sq] = pieces[SQ120(Mirror64[sq])];
    }

    reset();

    for(sq = 0; sq < 64; ++sq) {
        tp = SwapPiece[tempPiecesArray[sq]];
        pieces[SQ120(sq)] = tp;
    }

    side = tempSide;
    castlePerm = tempCastlePerm;
    enPas = tempEnPas;

    posKey = GeneratePosKey(this);

    updateListsMaterials();
    ASSERT(check());
}

// ---------- C API wrappers for backward compatibility ----------

int CheckBoard(Board *pos) { return pos->check(); }
void UpdateListsMaterials(Board *pos) { pos->updateListsMaterials(); }
void ResetBoard(Board *pos) { pos->reset(); }
int ParseFen(const char *fen, Board *pos) { return pos->parseFEN(fen); }
void PrintBoard(Board *pos) { pos->print(); }
void MirrorBoard(Board *pos) { pos->mirror(); }
