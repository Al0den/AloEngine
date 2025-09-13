#include "types.hpp"
#include <stdio.h>
#include <string.h>

#define INPUTBUFFER 400 * 6

void ParseGo(char *line, SearchInfo *info, Board *pos) {
    int depth = -1, movestogo = 50, movetime = -1;
    int time = -1, inc = 0;
    char *ptr = NULL;
    info->timeset = FALSE;

    if((ptr = strstr(line, "infinite"))) {
        ;
    } 

    if((ptr = strstr(line, "binc")) && pos->side == BLACK) {
        inc = atoi(ptr + 5);
    }

    if((ptr = strstr(line, "winc")) && pos->side == WHITE) {
        inc = atoi(ptr + 5);
    }

    if((ptr = strstr(line, "wtime")) && pos->side == WHITE) {
        time = atoi(ptr + 6) ;
    }

    if((ptr = strstr(line, "btime")) && pos->side == BLACK) {
        time = atoi(ptr + 6) ;
    }

    if((ptr = strstr(line, "movestogo"))) {
        movestogo = atoi(ptr + 10);
    }

    if((ptr = strstr(line, "movetime"))) {
        movetime = atoi(ptr + 9);
    }

    if((ptr = strstr(line, "depth"))) {
        depth = atoi(ptr + 6);
    }

    if(movetime != -1) {
        time = movetime;
        movestogo = 1;
    }

    info->startTime = GetTimeMS();
    info->depth = depth;

    if(time != -1) {
        info->timeset = TRUE;
        time /= movestogo;
        time -= 50;
        info->stopTime = info->startTime + time + inc/2;
    }

    if(depth == -1) {
        info->depth = MAX_DEPTH;
    }

    #ifdef DEBUG
    printf("time:%d start:%d stop:%d depth:%d timeset:%d\n", time, info->startTime, info->stopTime, info->depth, info->timeset);
    #endif
    SearchPosition(pos, info);
}

void ParsePosition(const char *lineIn, Board *pos) {
    lineIn += 9;
    const char *ptrChar = lineIn;
    if(strncmp(lineIn, "startpos", 8) == 0) {
        ParseFen(START_FEN, pos);
    } else {
        ptrChar = strstr(lineIn, "fen");
        if(ptrChar == NULL) {
            ParseFen(START_FEN, pos);
        } else {
            ptrChar += 4;
            ParseFen(ptrChar, pos);
        }
    }

    ptrChar = strstr(lineIn, "moves");
    int move;
    if(ptrChar != NULL) {
        ptrChar += 6;
        while(*ptrChar) {
            move = ParseMove(ptrChar, pos);
            if(move == NOMOVE) break;
            MakeMove(pos, move);
            pos->ply = 0;
            while(*ptrChar && *ptrChar != ' ') ptrChar++;
            ptrChar++;
        }
    }
    #ifdef DEBUG
    PrintBoard(pos);
    #endif
}

void Uci_Loop() {
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char line[INPUTBUFFER];
    // Respond to 'uci' command with ID, options, and uciok. Do not emit early.

    Board pos[1];
    SearchInfo info[1];
    InitHashTable(pos->HashTable);

    while(TRUE) {
        memset(&line[0], 0, sizeof(line));
        fflush(stdout);
        if(!fgets(line, INPUTBUFFER, stdin)) {
            continue;
        }
        
        if(line[0] == '\n') {
            continue;
        }

        if(!strncmp(line, "isready", 7)) {
            printf("readyok\n");
            continue;
        } else if(!strncmp(line, "position", 8)) {
            ParsePosition(line, pos);
        } else if(!strncmp(line, "ucinewgame", 10)) {
            ParsePosition("position startpos\n", pos);
        } else if(!strncmp(line, "go", 2)) {
            ParseGo(line, info, pos);
        } else if(!strncmp(line, "quit", 4)) {
            info->quit = TRUE;
            break;
        } else if(!strncmp(line, "uci", 3)) {
            printf("id name %s\n", "AlodenEngine");
            printf("id author %s\n", "Aloden");
            printf("uciok\n");
        } else if(!strncmp(line, "test", 4)) {
            PrintBitBoard(PawnShield[0]);
        } else if(!strncmp(line, "evaluate", 7)) {
            printf("Eval:%d\n", EvaluatePosition(pos));
        }
        if(info->quit) break;
    }
}
