// Minimal driver for perft testing
#include "alo/types.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    AllInit();

    int depth = 5; // default depth
    if (argc >= 2) {
        depth = std::atoi(argv[1]);
        if (depth <= 0) depth = 1;
    }

    Board pos[1];

    if (argc >= 3 && std::strcmp(argv[2], "fen") == 0) {
        std::string fen;
        for (int i = 3; i < argc; ++i) {
            if (!fen.empty()) fen.push_back(' ');
            fen += argv[i];
        }
        if (fen.empty()) {
            std::fprintf(stderr, "No FEN provided after 'fen' keyword. Using startpos.\n");
            ParseFen(START_FEN, pos);
        } else {
            ParseFen(fen.c_str(), pos);
        }
    } else {
        ParseFen(START_FEN, pos);
    }

    PerftTest(depth, pos);
    return 0;
}
