// src/nnue.cpp
#include "alo/nnue.hpp"
#include "alo/types.hpp"

#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

#include <cstdio>
#include <cstdlib>

static void nnue_fatal(const char* msg) {
    std::fprintf(stderr, "NNUE FATAL ERROR: %s\n", msg);
    std::fflush(stderr);
    std::abort();  // or std::terminate()
}

// --------- Config ---------

// Path to the weights file; adjust if needed
static const char* kNNUEWeightsPath = "data/nnue_weights.bin";

// Training normalization in Python: y = eval_cp / max_abs_cp
static constexpr float MAX_ABS_CP = 4410.0f;

// HalfKP constants (must match Python)
static constexpr int NUM_PIECES_NO_KING = 10;
static constexpr int FEATURES_PER_SIDE  = 64 * NUM_PIECES_NO_KING * 64;
static constexpr int TOTAL_FEATURES     = 2 * FEATURES_PER_SIDE;

// Map engine piece codes -> HalfKP piece index
// piece order in data.cpp: . P N B R Q K p n b r q k
// we want indices: P=0,N=1,B=2,R=3,Q=4,p=5,n=6,b=7,r=8,q=9
static constexpr int PieceToHalfKPIndex[13] = {
/*EMPTY*/ -1,
/*wP*/      0,
/*wN*/      1,
/*wB*/      2,
/*wR*/      3,
/*wQ*/      4,
/*wK*/     -1,
/*bP*/      5,
/*bN*/      6,
/*bB*/      7,
/*bR*/      8,
/*bQ*/      9,
/*bK*/     -1
};

// --------- Runtime weight storage ---------

static std::vector<float> g_embed_weight;  // [TOTAL_FEATURES, HIDDEN]
static std::vector<float> g_bias1;         // [HIDDEN]
static std::vector<float> g_fc2_weight;    // [FC2, HIDDEN]
static std::vector<float> g_fc2_bias;      // [FC2]
static std::vector<float> g_out_weight;    // [1, FC2]
static std::vector<float> g_out_bias;      // [1]

static int g_total_features = 0;
static int g_hidden_size    = 0;
static int g_fc2_size       = 0;

// --------- Binary loader (must match export_nnue_to_bin.py) ---------

static void read_exact(std::ifstream& in, void* dst, std::size_t n) {
    in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
    if (!in) {
        nnue_fatal("NNUE: failed to read from weights file");
    }
}

static void read_tensor(std::ifstream& in, std::vector<float>& dst, std::vector<int>& shape) {
    int32_t ndim = 0;
    read_exact(in, &ndim, sizeof(ndim));
    if (ndim < 0 || ndim > 4) {
        nnue_fatal("NNUE: invalid ndim");
    }

    shape.resize(ndim);
    for (int i = 0; i < ndim; ++i) {
        int32_t dim = 0;
        read_exact(in, &dim, sizeof(dim));
        if (dim <= 0) {
            nnue_fatal("NNUE: invalid dimension");
        }
        shape[i] = dim;
    }

    std::size_t total = 1;
    for (int d : shape) {
        total *= static_cast<std::size_t>(d);
    }

    dst.resize(total);
    read_exact(in, dst.data(), total * sizeof(float));
}

static void NNUE_LoadWeights(const char* path = kNNUEWeightsPath) {
    static bool loaded = false;
    if (loaded) return;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        nnue_fatal("NNUE: cannot open weights file");
    }

    // Magic "NNUE"
    char magic[4];
    read_exact(in, magic, 4);
    if (std::memcmp(magic, "NNUE", 4) != 0) {
        nnue_fatal("NNUE: bad magic in weights file");
    }

    int32_t version = 0;
    read_exact(in, &version, sizeof(version));
    if (version != 1) {
        nnue_fatal("NNUE: unsupported weights version");
    }

    int32_t num_tensors = 0;
    read_exact(in, &num_tensors, sizeof(num_tensors));
    if (num_tensors != 6) {
        nnue_fatal("NNUE: expected 6 tensors in weights file");
    }

    std::vector<int> shape;

    // 1) embed.weight
    read_tensor(in, g_embed_weight, shape);
    if (shape.size() != 2) {
        nnue_fatal("NNUE: embed.weight must be 2D");
    }
    g_total_features = shape[0];
    g_hidden_size    = shape[1];

    if (g_total_features != TOTAL_FEATURES) {
        nnue_fatal("NNUE: TOTAL_FEATURES mismatch vs weights file");
    }

    // 2) bias1
    read_tensor(in, g_bias1, shape);
    if (shape.size() != 1 || shape[0] != g_hidden_size) {
        nnue_fatal("NNUE: bias1 shape mismatch");
    }

    // 3) fc2.weight
    read_tensor(in, g_fc2_weight, shape);
    if (shape.size() != 2) {
        nnue_fatal("NNUE: fc2.weight must be 2D");
    }
    g_fc2_size = shape[0];
    if (shape[1] != g_hidden_size) {
        nnue_fatal("NNUE: fc2.weight HIDDEN mismatch");
    }

    // 4) fc2.bias
    read_tensor(in, g_fc2_bias, shape);
    if (shape.size() != 1 || shape[0] != g_fc2_size) {
        nnue_fatal("NNUE: fc2.bias shape mismatch");
    }

    // 5) out.weight
    read_tensor(in, g_out_weight, shape);
    if (shape.size() != 2 || shape[0] != 1 || shape[1] != g_fc2_size) {
        nnue_fatal("NNUE: out.weight shape mismatch");
    }

    // 6) out.bias
    read_tensor(in, g_out_bias, shape);
    if (shape.size() != 1 || shape[0] != 1) {
        nnue_fatal("NNUE: out.bias shape mismatch");
    }

    loaded = true;
}

// --------- HalfKP feature encoding from Board ---------

// equivalent to Python _mirror_square_vertical on 0..63 index
static inline int mirror_square_64(int sq) {
    int rank = sq / 8;
    int file = sq % 8;
    int mirrored_rank = 7 - rank;
    return mirrored_rank * 8 + file;
}

static inline int feature_index_for_side(
    int king_sq, int piece_idx, int piece_sq, int side_index
) {
    // same as Python _feature_index_for_side
    const int base  = side_index * FEATURES_PER_SIDE;
    const int inner = king_sq * (NUM_PIECES_NO_KING * 64)
                    + piece_idx * 64
                    + piece_sq;
    return base + inner;
}

// Build active feature indices for this Board (non-incremental)
static void encode_halfkp_features(const Board* pos, std::vector<int>& feats) {
    feats.clear();
    feats.reserve(64 * 2); // rough upper bound

    const int whiteKingSq64 = SQ64(pos->kingSq[WHITE]);
    const int blackKingSq64 = SQ64(pos->kingSq[BLACK]);

    const int king_w_persp  = whiteKingSq64;
    const int king_b_persp  = mirror_square_64(blackKingSq64);

    // loop over all non-king pieces
    for (int piece = wP; piece <= bQ; ++piece) {
        int hp_idx = PieceToHalfKPIndex[piece];
        if (hp_idx < 0) continue;

        const int num = pos->pceNum[piece];
        for (int i = 0; i < num; ++i) {
            const int sq120 = pos->plist[piece][i];
            const int sq64  = SQ64(sq120);

            // white perspective
            {
                int feat = feature_index_for_side(
                    king_w_persp,
                    hp_idx,
                    sq64,
                    0
                );
                feats.push_back(feat);
            }

            // black perspective (board mirrored vertically)
            {
                int sq_persp = mirror_square_64(sq64);
                int feat = feature_index_for_side(
                    king_b_persp,
                    hp_idx,
                    sq_persp,
                    1
                );
                feats.push_back(feat);
            }
        }
    }
}

// --------- Forward pass ---------

int NNUE_Evaluate(const Board* pos) {
    // 1) Load weights once
    NNUE_LoadWeights();

    // 2) Build feature list
    std::vector<int> feats;
    encode_halfkp_features(pos, feats);

    const int HIDDEN = g_hidden_size;
    const int FC2    = g_fc2_size;

    // 3) First layer: EmbeddingBag SUM + bias1 + ReLU
    std::vector<float> h1(HIDDEN);

    // init with bias1
    for (int i = 0; i < HIDDEN; ++i) {
        h1[i] = g_bias1[i];
    }

    // sum embeddings of active features
    for (int f : feats) {
        if (f < 0 || f >= g_total_features) continue;  // safety
        const float* w = &g_embed_weight[static_cast<std::size_t>(f) * HIDDEN];
        for (int i = 0; i < HIDDEN; ++i) {
            h1[i] += w[i];
        }
    }

    // ReLU
    for (int i = 0; i < HIDDEN; ++i) {
        if (h1[i] < 0.0f) h1[i] = 0.0f;
    }

    // 4) Second layer: fc2 + ReLU
    std::vector<float> h2(FC2);
    for (int j = 0; j < FC2; ++j) {
        float sum = g_fc2_bias[j];
        const float* wrow = &g_fc2_weight[static_cast<std::size_t>(j) * HIDDEN];
        for (int i = 0; i < HIDDEN; ++i) {
            sum += wrow[i] * h1[i];
        }
        h2[j] = std::max(0.0f, sum);
    }

    // 5) Output layer: out.weight [1, FC2], out.bias[1]
    float out = g_out_bias[0];
    for (int j = 0; j < FC2; ++j) {
        out += g_out_weight[j] * h2[j];
    }

    // 6) Convert back to cp (reverse normalization)
    float eval_white = out * MAX_ABS_CP;
    int eval_cp = static_cast<int>(std::lround(eval_white));

    // flip for side to move
    if (pos->side == BLACK) {
        eval_cp = -eval_cp;
    }

    return eval_cp;
}
