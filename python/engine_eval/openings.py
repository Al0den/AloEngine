"""
openings.py
-----------
Random opening selection from a Polyglot (.bin) book.

Usage:
    from engine_eval.openings import random_polyglot_opening

    moves = random_polyglot_opening("data/engine_eval/book/UHO_30_v1.bin",
                                    max_depth=10)
    print(moves)  # ['e2e4', 'c7c5', 'g1f3', ...]
"""

import chess
import chess.polyglot
import random
from pathlib import Path


def random_polyglot_opening(book_path: str | Path, max_depth: int = 12) -> list[str]:
    """
    Select a random opening line from a Polyglot opening book (.bin).

    Args:
        book_path   (str | Path): Path to .bin opening book (UHO, 8move, Perfect, etc.)
        max_depth   (int): maximum number of moves to extract

    Returns:
        List[str]: list of UCI moves, e.g. ["e2e4", "c7c5", "g1f3"]

    Notes:
        - Works with any polyglot book (Perfect.bin, 8move.bin, UHO).
        - Randomizes each step → highly variable openings.
        - Uses only legal book moves.
        - Returns [] if book is empty or no move is found.
    """
    book_path = Path(book_path)
    if not book_path.exists():
        raise FileNotFoundError(f"Opening book not found: {book_path}")

    board = chess.Board()
    out_moves: list[str] = []

    with chess.polyglot.open_reader(str(book_path)) as reader:
        # Gather all first-move entries
        entries = list(reader.find_all(board))

        if not entries:
            return []

        # Select a random first move
        entry = random.choice(entries)
        move = entry.move
        out_moves.append(move.uci())
        board.push(move)

        # Continue selecting random moves from the book until max_depth
        for _ in range(max_depth - 1):
            entries = list(reader.find_all(board))
            if not entries:
                break
            entry = random.choice(entries)
            move = entry.move
            out_moves.append(move.uci())
            board.push(move)

    return out_moves


def random_opening_fixed_depth(book_path: str | Path, depth: int = 8) -> list[str]:
    """
    Select a random book move sequence of EXACT depth (or stop earlier if exhausted).
    Same as random_polyglot_opening, but depth is fixed.

    Args:
        book_path: path to the .bin book
        depth: number of plies (moves) to sample

    Returns:
        List[str] of length <= depth
    """
    return random_polyglot_opening(book_path, max_depth=depth)


def test_random_opening():
    """Quick manual test."""
    print("Testing random Polyglot opening...")
    moves = random_polyglot_opening("data/engine_eval/book/UHO_30_v1.bin", 10)
    print("Moves:", moves)


if __name__ == "__main__":
    test_random_opening()
