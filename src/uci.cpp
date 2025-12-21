#include "alo/types.hpp"
#include "alo/uci.hpp"
#include <stdio.h>
#include <string.h>

#define INPUTBUFFER 400 * 6

// Minimal UCI class wrapper to structure logic
namespace alo {
void Uci::parseGo(char *line, SearchInfo *info, Board *pos) {
        int depth = -1, movestogo = 50, movetime = -1;
        int time = -1, inc = 0;
        char *ptr = NULL;
        info->timeset = FALSE;

        if((ptr = strstr(line, "infinite"))) {
            ;
        }
        if((ptr = strstr(line, "binc")) && pos->getSide() == BLACK) inc = atoi(ptr + 5);
        if((ptr = strstr(line, "winc")) && pos->getSide() == WHITE) inc = atoi(ptr + 5);
        if((ptr = strstr(line, "wtime")) && pos->getSide() == WHITE) time = atoi(ptr + 6);
        if((ptr = strstr(line, "btime")) && pos->getSide() == BLACK) time = atoi(ptr + 6);
        if((ptr = strstr(line, "movestogo"))) movestogo = atoi(ptr + 10);
        if((ptr = strstr(line, "movetime"))) movetime = atoi(ptr + 9);
        if((ptr = strstr(line, "depth"))) depth = atoi(ptr + 6);

        if(movetime != -1) { time = movetime; movestogo = 1; }

        info->startTime = GetTimeMS();
        info->depth = depth;

        if(time != -1) {
            info->timeset = TRUE;
            time /= movestogo; time -= 50;
            info->stopTime = info->startTime + time + inc/2;
        }
        if(depth == -1) info->depth = MAX_DEPTH;
        SearchPosition(pos, info);
}

void Uci::parsePosition(const char *lineIn, Board *pos) {
        lineIn += 9;
        const char *ptrChar = lineIn;
        if(strncmp(lineIn, "startpos", 8) == 0) {
            pos->parseFEN(START_FEN);
        } else {
            ptrChar = strstr(lineIn, "fen");
            if(ptrChar == NULL) {
                pos->parseFEN(START_FEN);
            } else {
                ptrChar += 4;
                pos->parseFEN(ptrChar);
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
}

void Uci::loop() {
        setbuf(stdin, NULL);
        setbuf(stdout, NULL);

        char line[INPUTBUFFER];

        Board pos[1];
        SearchInfo info[1];
        InitHashTable(pos->HashTable);

        while(TRUE) {
            memset(&line[0], 0, sizeof(line));
            fflush(stdout);
            if(!fgets(line, INPUTBUFFER, stdin)) {
                continue;
            }
            if(line[0] == '\n') continue;

            if(!strncmp(line, "isready", 7)) {
                printf("readyok\n");
                continue;
            } else if(!strncmp(line, "setoption", 9)) {
                // parse: setoption name <Name> value <Value>
                char *pName = strstr(line, "name ");
                char *pValue = strstr(line, "value ");
                if(pName) pName += 5; if(pValue) pValue += 6;
                if(pName) {
                    char *nl = strchr(pName, '\n'); if(nl) *nl = 0;
                    char nameBuf[128]; memset(nameBuf, 0, sizeof(nameBuf));
                    if(pValue) {
                        size_t len = (size_t)(pValue - pName - 6);
                        if(len >= sizeof(nameBuf)) len = sizeof(nameBuf)-1;
                        strncpy(nameBuf, pName, len);
                    } else {
                        strncpy(nameBuf, pName, sizeof(nameBuf)-1);
                    } 
                    if(!strncmp(nameBuf, "Hash", 4)) {
                        if(pValue) {
                            int mb = atoi(pValue);
                            if (mb < 1) mb = 1; if (mb > 4096) mb = 4096;
                            ReInitHashTable(pos->HashTable, mb);
                        }
                    } else if(!strncmp(nameBuf, "Clear Hash", 10)) {
                        ClearHashTable(pos->HashTable);
                    } else if(!strncmp(nameBuf, "Threads", 7)) {
                        // single-threaded for now; ignore
                    }
                }
                continue;
            } else if(!strncmp(line, "position", 8)) {
                parsePosition(line, pos);
            } else if(!strncmp(line, "ucinewgame", 10)) {
                parsePosition("position startpos\n", pos);
                ClearHashTable(pos->HashTable);
            } else if(!strncmp(line, "go", 2)) {
                parseGo(line, info, pos);
            } else if(!strncmp(line, "quit", 4)) {
                info->quit = TRUE; break;
            } else if(!strncmp(line, "uci", 3)) {
                printf("id name %s\n", "AlodenEngine");
                printf("id author %s\n", "Aloden");
                printf("option name Hash type spin default 256 min 1 max 4096\n");
                printf("option name Clear Hash type button\n");
                printf("option name Threads type spin default 1 min 1 max 1\n");
                printf("uciok\n");
            } else if(!strncmp(line, "test", 4)) {
                PrintBitBoard(PawnShield[0]);
            } else if(!strncmp(line, "evaluate", 7)) {
                printf("Eval:%d\n", EvaluatePosition(pos));
            }
            if(info->quit) break;
        }
}
} // namespace alo

// Free-function delegates for compatibility
void ParseGo(char *line, SearchInfo *info, Board *pos) { static alo::Uci u; u.parseGo(line, info, pos); }

void ParsePosition(const char *lineIn, Board *pos) { static alo::Uci u; u.parsePosition(lineIn, pos); }

void Uci_Loop() { static alo::Uci u; u.loop(); }
