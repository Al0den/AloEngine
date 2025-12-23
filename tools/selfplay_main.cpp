// selfplay_main.cpp
#include "alo/types.hpp"
#include "alo/search.hpp"
#include "alo/tt.hpp"       // for InitHashTable, ISMATE
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <random>
#include <fstream>
#include <string>

// Convert Board -> FEN string
static std::string boardToFEN(const Board *pos) {
    char buf[128];
    if (pos->toFEN(buf, sizeof(buf)) != 0) {
        return std::string(); // empty on error
    }
    return std::string(buf);
}

// Map internal search score -> centipawn label (clamped)
static int scoreToLabelCp(int score, int maxCp = 4410) {
    if (score > ISMATE)  return +maxCp;   // mate in favor
    if (score < -ISMATE) return -maxCp;   // mate against

    if (score >  maxCp) score =  maxCp;
    if (score < -maxCp) score = -maxCp;
    return score;
}

// Record one position as JSON line
static void writePositionJson(std::ofstream &out,
                              const Board *pos,
                              int gameId,
                              int ply,
                              int evalCp) {
    std::string fen = boardToFEN(pos);
    if (fen.empty()) return;

    out << "{"
        << "\"game\":" << gameId << ","
        << "\"ply\":" << ply << ","
        << "\"side\":\"" << (pos->side == WHITE ? "w" : "b") << "\","
        << "\"fen\":\"" << fen << "\","
        << "\"eval\":" << evalCp
        << "}\n";
}

// Play one self-play game, using an existing Board+SearchInfo (with TT already inited)
static int playOneGame(std::ofstream &out,
                       int gameId,
                       std::mt19937 &rng,
                       int minRandomMovesPerSide,
                       int maxRandomMovesPerSide,
                       int maxPlies,
                       int labelDepth,
                       int labelTimeMs,
                       Board *pos,
                       SearchInfo *info) {
    // Reset position to startpos
    ParseFen(START_FEN, pos);

    std::uniform_int_distribution<int> randSideMoves(minRandomMovesPerSide,
                                                     maxRandomMovesPerSide);
    int randomMovesPerSide = randSideMoves(rng);
    int randomPlies = 2 * randomMovesPerSide; // both sides

    int recorded = 0;
    int ply = 0;

    MoveList list[1];

    auto randomInt = [&](int hi) {
        std::uniform_int_distribution<int> d(0, hi - 1);
        return d(rng);
    };

    // ------------------- RANDOM PHASE -------------------
    while (ply < randomPlies && ply < maxPlies) {
        GenerateAllMoves(pos, list);
        if (list->count == 0) break;

        // Label with search score
        int s = SearchScore(pos, info, labelDepth, labelTimeMs);
        int evalCp = scoreToLabelCp(s);
        writePositionJson(out, pos, gameId, ply, evalCp);
        ++recorded;

        int idx = randomInt(list->count);
        int move = list->moves[idx].move;
        if (!MakeMove(pos, move)) break;

        ++ply;
    }

    // ------------------- SELF-PLAY PHASE -------------------
    while (ply < maxPlies) {
        GenerateAllMoves(pos, list);
        if (list->count == 0) break;  // checkmate or stalemate

        // Label with search score
        int s = SearchScore(pos, info, labelDepth, labelTimeMs);
        int evalCp = scoreToLabelCp(s);
        writePositionJson(out, pos, gameId, ply, evalCp);
        ++recorded;

        // Very cheap move selection: 1-ply using static eval
        int bestMove = NOMOVE;
        int bestScore = -SCORE_INF;

        for (int i = 0; i < list->count; ++i) {
            int move = list->moves[i].move;
            if (!MakeMove(pos, move)) continue;

            int childEval = EvaluatePosition(pos); // from new side's POV
            int scoreForCurrentSide = -childEval;  // flip back

            TakeMove(pos);

            if (bestMove == NOMOVE || scoreForCurrentSide > bestScore) {
                bestScore = scoreForCurrentSide;
                bestMove  = move;
            }
        }

        if (bestMove == NOMOVE) break;
        if (!MakeMove(pos, bestMove)) break;

        ++ply;
    }

    return recorded;
}

int main(int argc, char **argv) {
    int numGames = 100000;
    const char *outFile = "data/nnue_dataset.jsonl";
    int minRandomPerSide = 5;
    int maxRandomPerSide = 7;
    int maxPlies = 50;
    int labelDepth = 4;
    int labelTimeMs = 0;

    if (argc >= 2) numGames = std::atoi(argv[1]);
    if (argc >= 3) outFile = argv[2];
    if (argc >= 4) minRandomPerSide = std::atoi(argv[3]);
    if (argc >= 5) maxRandomPerSide = std::atoi(argv[4]);
    if (argc >= 6) maxPlies = std::atoi(argv[5]);
    if (argc >= 7) labelDepth = std::atoi(argv[6]);
    if (argc >= 8) labelTimeMs = std::atoi(argv[7]);

    if (numGames <= 0) numGames = 1;
    if (minRandomPerSide < 0) minRandomPerSide = 0;
    if (maxRandomPerSide < minRandomPerSide)
        maxRandomPerSide = minRandomPerSide;

    srand(0);
    AllInit();

    Board pos[1];
    SearchInfo info[1];
    ResetBoard(pos);            
    InitHashTable(pos->HashTable);     

    std::mt19937 rng(123456u);

    std::ofstream out(outFile, std::ios::out | std::ios::trunc);
    if (!out) {
        std::fprintf(stderr, "Cannot open output file: %s\n", outFile);
        return 1;
    }

    int totalPositions = 0;
    for (int g = 0; g < numGames; ++g) {
        int rec = playOneGame(out, g, rng,
                              minRandomPerSide,
                              maxRandomPerSide,
                              maxPlies,
                              labelDepth,
                              labelTimeMs,
                              pos,
                              info);
        totalPositions += rec;
        std::fprintf(stderr, "Game %d: %d positions\n", g, rec);
    }

    std::fprintf(stderr, "Done. Total positions: %d\n", totalPositions);
    return 0;
}
