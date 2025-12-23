import chess
import chess.pgn
from pathlib import Path
from typing import Tuple
import random
import datetime

from uci_engine import UCIEngine  # your existing wrapper
from config import PGN_DIR, BOOK_FILE
from openings import random_polyglot_opening


class PygameBoardRenderer:
    def __init__(self, title: str, square_size: int = 80):
        try:
            import pygame  # type: ignore
        except Exception as exc:
            raise RuntimeError(
                "pygame is required for the visual board. "
                "Install with: python -m pip install --break-system-packages --user pygame"
            ) from exc
        self.pygame = pygame
        self.square_size = square_size
        self.board_px = square_size * 8
        self.status_px = 40
        self.screen = None
        self.font = None
        self.status_font = None
        self.title = title
        self.light = (240, 217, 181)
        self.dark = (181, 136, 99)
        self.highlight = (246, 246, 105)
        self.white_piece = (245, 245, 245)
        self.black_piece = (40, 40, 40)
        self.status_bg = (30, 30, 30)
        self.status_fg = (230, 230, 230)
        self._init_window()

    def _init_window(self) -> None:
        pygame = self.pygame
        pygame.init()
        pygame.font.init()
        self.screen = pygame.display.set_mode((self.board_px, self.board_px + self.status_px))
        pygame.display.set_caption(self.title)
        self.font = pygame.font.SysFont("DejaVu Sans Mono", int(self.square_size * 0.6))
        self.status_font = pygame.font.SysFont("DejaVu Sans Mono", 18)

    def close(self) -> None:
        pygame = self.pygame
        pygame.display.quit()
        pygame.quit()

    def _square_rect(self, file: int, rank: int):
        x = file * self.square_size
        y = (7 - rank) * self.square_size
        return self.pygame.Rect(x, y, self.square_size, self.square_size)

    def _draw_square(self, file: int, rank: int, color):
        pygame = self.pygame
        rect = self._square_rect(file, rank)
        pygame.draw.rect(self.screen, color, rect)

    def _draw_piece(self, piece: chess.Piece, file: int, rank: int):
        pygame = self.pygame
        symbol = piece.symbol()
        color = self.white_piece if piece.color == chess.WHITE else self.black_piece
        text = self.font.render(symbol, True, color)
        rect = text.get_rect(center=self._square_rect(file, rank).center)
        self.screen.blit(text, rect)

    def _draw_status(self, board: chess.Board, last_move: str | None, result: str | None):
        pygame = self.pygame
        y = self.board_px
        pygame.draw.rect(self.screen, self.status_bg, (0, y, self.board_px, self.status_px))
        side = "White" if board.turn == chess.WHITE else "Black"
        status = f"To move: {side} | Move: {board.fullmove_number}"
        if last_move:
            status += f" | Last: {last_move}"
        if result:
            status += f" | Result: {result}"
        text = self.status_font.render(status, True, self.status_fg)
        self.screen.blit(text, (8, y + 10))

    def update(
        self,
        board: chess.Board,
        last_move: str | None,
        result: str | None,
    ) -> bool:
        pygame = self.pygame
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return False

        self.screen.fill(self.status_bg)

        highlight_squares = set()
        if last_move:
            try:
                move = chess.Move.from_uci(last_move)
                highlight_squares.add(move.from_square)
                highlight_squares.add(move.to_square)
            except ValueError:
                pass

        for rank in range(8):
            for file in range(8):
                is_light = (file + rank) % 2 == 0
                color = self.light if is_light else self.dark
                square = chess.square(file, rank)
                if square in highlight_squares:
                    color = self.highlight
                self._draw_square(file, rank, color)

        for rank in range(8):
            for file in range(8):
                piece = board.piece_at(chess.square(file, rank))
                if piece:
                    self._draw_piece(piece, file, rank)

        self._draw_status(board, last_move, result)
        pygame.display.flip()
        return True


def play_single_game(
    eng_a_info,
    eng_b_info,
    movetime_ms: int = 500,
    book_max_depth: int = 10,
    display_backend: str | None = "pygame",
    game_label: str | None = None,
    engine_log_io: bool = False,
) -> Tuple[float, str, str]:
    """
    Play one game between eng_a and eng_b.

    Returns:
      result_a (1.0, 0.5, 0.0),
      pgn_path (str),
      color_a ("white" or "black")
    """
    board = chess.Board()

    # --- Choose random opening line from book and apply it ---
    try:
        opening_moves = random_polyglot_opening(BOOK_FILE, max_depth=book_max_depth)
    except FileNotFoundError:
        opening_moves = []  # fallback: pure startpos if book missing

    for uci in opening_moves:
        move = board.parse_uci(uci)
        if move not in board.legal_moves:
            break
        board.push(move)

    # Randomise which engine is white
    a_is_white = random.choice([True, False])

    white_info = eng_a_info if a_is_white else eng_b_info
    black_info = eng_b_info if a_is_white else eng_a_info

    w = UCIEngine(
        white_info.path, white_info.options, name=white_info.name, log_io=engine_log_io
    )
    b = UCIEngine(
        black_info.path, black_info.options, name=black_info.name, log_io=engine_log_io
    )

    w.start()
    b.start()
    w.new_game()
    b.new_game()

    game = chess.pgn.Game()
    game.headers["White"] = white_info.name
    game.headers["Black"] = black_info.name
    game.headers["Result"] = "*"
    if opening_moves:
        game.headers["Opening"] = "book"
        game.headers["FEN"] = board.fen()
        game.headers["StartMoves"] = " ".join(opening_moves)

    node = game

    last_move = None
    renderer = None
    if display_backend == "pygame":
        renderer = PygameBoardRenderer(game_label or f"{white_info.name} vs {black_info.name}")
    try:
        while not board.is_game_over():
            if renderer:
                if not renderer.update(board, last_move, None):
                    break
            if board.turn == chess.WHITE:
                move_uci = w.choose_move(board, movetime_ms)
            else:
                move_uci = b.choose_move(board, movetime_ms)

            move = board.parse_uci(move_uci)
            board.push(move)
            last_move = move_uci
            node = node.add_variation(move)
    finally:
        w.stop()
        b.stop()

    result_str = board.result()
    game.headers["Result"] = result_str
    if renderer:
        renderer.update(board, last_move, result_str)
        renderer.close()

    # Map to result from A's perspective
    if result_str == "1-0":
        result_a = 1.0 if a_is_white else 0.0
    elif result_str == "0-1":
        result_a = 0.0 if a_is_white else 1.0
    else:
        result_a = 0.5

    color_a = "white" if a_is_white else "black"

    # Save PGN (add timestamp so you don't overwrite)
    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    PGN_DIR.mkdir(parents=True, exist_ok=True)
    pgn_name = f"{ts}_{white_info.name}_vs_{black_info.name}.pgn"
    pgn_path = (PGN_DIR / pgn_name).as_posix()
    with open(pgn_path, "w") as f:
        print(game, file=f)

    return result_a, pgn_path, color_a
