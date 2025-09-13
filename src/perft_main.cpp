// Minimal driver for perft testing
#include "types.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
    AllInit();

    int depth = 5; // default depth
    if (argc >= 2) {
        depth = std::atoi(argv[1]);
        if (depth <= 0) depth = 1;
    }

    Board pos[1];

    // Default position: startpos; allow optional FEN via arguments
    // Usage examples:
    //   perft 5                 -> startpos depth 5
    //   perft 4 fen rnbqkbnr/... w KQkq - 0 1
        if (argc >= 3 && std::strcmp(argv[2], "fen") == 0) {
        // Concatenate remaining args as FEN with spaces
        static char fen[256] = {0};
        fen[0] = '\0';
        for (int i = 3; i < argc; ++i) {
            std::strcat(fen, argv[i]);
            if (i + 1 < argc) std::strcat(fen, " ");
        }
        if (fen[0] == '\0') {
            std::fprintf(stderr, "No FEN provided after 'fen' keyword. Using startpos.\n");
            ParseFen(START_FEN, pos);
        } else {
            ParseFen(fen, pos);
        }
    } else {
        ParseFen(START_FEN, pos);
    }

    PerftTest(depth, pos);
    return 0;
}
