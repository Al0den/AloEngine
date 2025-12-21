from typing import List

# --- Constants for the HalfKP encoding ---

# P, N, B, R, Q, p, n, b, r, q  (no kings!)
PIECE_TO_INDEX = {
    "P": 0,
    "N": 1,
    "B": 2,
    "R": 3,
    "Q": 4,
    "p": 5,
    "n": 6,
    "b": 7,
    "r": 8,
    "q": 9,
}

NUM_PIECES_NO_KING = 10
FEATURES_PER_SIDE = 64 * NUM_PIECES_NO_KING * 64
TOTAL_FEATURES = 2 * FEATURES_PER_SIDE  # 81920


def _square_index_from_fen(rank_idx: int, file_idx: int) -> int:
    """
    Convert rank/file (0-based, rank 0 = '1', file 0 = 'a') to 0..63 with a1 = 0.
    FEN lists ranks 8->1; rank_idx is 0..7 for those ranks but *we* treat 0='8'.
    """
    # rank_idx: 0 (rank 8) .. 7 (rank 1)
    board_rank_from_bottom = 7 - rank_idx  # 0='1' .. 7='8'
    return board_rank_from_bottom * 8 + file_idx


def _mirror_square_vertical(sq: int) -> int:
    """
    Mirror square vertically (flip ranks) around horizontal axis.
    a1(0) <-> a8(56), etc.
    """
    rank = sq // 8
    file = sq % 8
    mirrored_rank = 7 - rank
    return mirrored_rank * 8 + file


def _feature_index_for_side(
    king_sq: int,
    piece_idx: int,
    piece_sq: int,
    side_index: int,  # 0 = white perspective, 1 = black perspective
) -> int:
    """
    Compute global feature index in [0, TOTAL_FEATURES) for HalfKP.
    side_index selects which half (white/black perspective).
    """
    assert 0 <= king_sq < 64
    assert 0 <= piece_sq < 64
    assert 0 <= piece_idx < NUM_PIECES_NO_KING
    assert side_index in (0, 1)

    base = side_index * FEATURES_PER_SIDE
    # Layout: king_sq * (num_piece_types * 64) + piece_idx * 64 + piece_sq
    inner = king_sq * (NUM_PIECES_NO_KING * 64) + piece_idx * 64 + piece_sq
    return base + inner


def fen_to_features(fen: str) -> List[int]:
    """
    Convert a FEN string into a list of active HalfKP feature indices.

    Encoding:
      - Two perspectives: white and black.
      - For each side S in {white, black}:
          * Fix king square of S (from board).
          * For each non-king piece (both colors) on the board:
              - Transform coordinates from S's perspective
                (black perspective sees the board vertically flipped).
              - Create feature (S, king_sq_S, piece_type, piece_sq).
      - Pieces K/k (kings) are NOT used as 'P'-type features (only as K in HalfK).

    Returns:
      A list of integers in [0, TOTAL_FEATURES) representing active features.
    """
    board_part = fen.split()[0]
    ranks = board_part.split("/")

    if len(ranks) != 8:
        raise ValueError(f"Invalid FEN, expected 8 ranks: {fen}")

    # Board: list of 64 chars ('.' for empty)
    board = ["." for _ in range(64)]
    white_king_sq = None
    black_king_sq = None

    # Parse piece placement
    for rank_idx, rank_str in enumerate(ranks):
        file_idx = 0
        for ch in rank_str:
            if ch.isdigit():
                file_idx += int(ch)
            else:
                if file_idx >= 8:
                    raise ValueError(f"Too many files in rank {rank_idx} of FEN: {fen}")
                sq = _square_index_from_fen(rank_idx, file_idx)
                board[sq] = ch
                if ch == "K":
                    white_king_sq = sq
                elif ch == "k":
                    black_king_sq = sq
                file_idx += 1

        if file_idx != 8:
            raise ValueError(f"Rank {rank_idx} doesn't sum to 8 in FEN: {fen}")

    if white_king_sq is None or black_king_sq is None:
        raise ValueError("FEN must contain both kings for HalfKP features.")

    features: List[int] = []

    # Pre-collect piece squares (excluding kings)
    pieces = []  # (piece_char, square)
    for sq, ch in enumerate(board):
        if ch in ("K", "k") or ch == ".":
            continue
        if ch not in PIECE_TO_INDEX:
            # Ignore unknown / fairy pieces if any
            continue
        pieces.append((ch, sq))

    # White perspective (side_index = 0)
    king_w = white_king_sq
    for ch, sq in pieces:
        p_idx = PIECE_TO_INDEX[ch]
        sq_persp = sq  # white sees actual board
        ksq_persp = king_w
        feat = _feature_index_for_side(
            king_sq=ksq_persp,
            piece_idx=p_idx,
            piece_sq=sq_persp,
            side_index=0,
        )
        features.append(feat)

    # Black perspective (side_index = 1)
    king_b = black_king_sq
    king_b_persp = _mirror_square_vertical(king_b)
    for ch, sq in pieces:
        p_idx = PIECE_TO_INDEX[ch]
        sq_persp = _mirror_square_vertical(sq)
        feat = _feature_index_for_side(
            king_sq=king_b_persp,
            piece_idx=p_idx,
            piece_sq=sq_persp,
            side_index=1,
        )
        features.append(feat)

    return features