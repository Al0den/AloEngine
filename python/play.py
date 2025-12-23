#!/usr/bin/env python3
import subprocess
import sys
import threading
import queue
import time

import pygame
import chess

# ====== CONFIG ======

ENGINE_PATH = "./build/AloEngine"   # <-- change if needed
ENGINE_MOVETIME_MS = 1000            # engine thinking time per move

BOARD_SIZE_PX = 640
SQUARE_SIZE = BOARD_SIZE_PX // 8

LIGHT_COLOR = (240, 217, 181)
DARK_COLOR  = (181, 136, 99)
HIGHLIGHT_COLOR = (186, 202, 68)
LASTMOVE_COLOR = (246, 246, 105)
BG_COLOR = (30, 30, 30)

# Unicode chess pieces
PIECE_UNICODE = {
    chess.PAWN:   ("♙", "♟"),
    chess.KNIGHT: ("♘", "♞"),
    chess.BISHOP: ("♗", "♝"),
    chess.ROOK:   ("♖", "♜"),
    chess.QUEEN:  ("♕", "♛"),
    chess.KING:   ("♔", "♚"),
}


# ====== UCI Engine Wrapper ======

class UCIEngine:
    def __init__(self, path: str, verbose: bool = True):
        self.path = path
        self.proc = None
        self.stdout_thread = None
        self.stdout_queue = queue.Queue()
        self.running = False
        self.verbose = verbose

    def start(self):
        if self.verbose:
            print(f"[ENGINE] Starting process: {self.path}", flush=True)

        self.proc = subprocess.Popen(
            [self.path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        self.running = True
        self.stdout_thread = threading.Thread(target=self._reader_loop, daemon=True)
        self.stdout_thread.start()

        self._send("uci")
        self._wait_for("uciok")
        self._send("isready")
        self._wait_for("readyok")

    def _reader_loop(self):
        while self.running and self.proc and self.proc.stdout:
            line = self.proc.stdout.readline()
            if not line:
                break
            line = line.strip()
            if self.verbose:
                print(f"[ENGINE <<] {line}", flush=True)
            self.stdout_queue.put(line)

    def _send(self, cmd: str):
        if not self.proc or not self.proc.stdin:
            raise RuntimeError("Engine process not running")
        if self.verbose:
            print(f"[ENGINE >>] {cmd}", flush=True)
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

    def _wait_for(self, token: str, timeout: float = 5.0):
        end = time.time() + timeout
        while time.time() < end:
            try:
                line = self.stdout_queue.get(timeout=0.1)
            except queue.Empty:
                continue
            if token in line:
                return
        raise TimeoutError(f"Timed out waiting for '{token}'")

    def new_game(self):
        if self.verbose:
            print("[INFO] New game", flush=True)
        self._send("ucinewgame")
        self._send("isready")
        self._wait_for("readyok")

    def set_position(self, moves_uci):
        if moves_uci:
            moves_str = " ".join(moves_uci)
            cmd = f"position startpos moves {moves_str}"
        else:
            cmd = "position startpos"
        self._send(cmd)

    def go(self, movetime_ms: int = ENGINE_MOVETIME_MS):
        # Flush old lines
        while not self.stdout_queue.empty():
            try:
                _ = self.stdout_queue.get_nowait()
            except queue.Empty:
                break

        self._send(f"go movetime {movetime_ms}")

        bestmove = None
        while True:
            line = self.stdout_queue.get()  # blocking
            # all lines already logged in _reader_loop
            if line.startswith("bestmove"):
                parts = line.split()
                if len(parts) >= 2:
                    bestmove = parts[1]
                break
        return bestmove

    def stop(self):
        if not self.proc:
            return
        try:
            self._send("quit")
        except Exception:
            pass
        self.running = False
        try:
            self.proc.terminate()
        except Exception:
            pass
        self.proc = None
        if self.verbose:
            print("[ENGINE] Stopped", flush=True)


# ====== GUI helpers ======

def square_from_mouse(pos):
    x, y = pos
    file = x // SQUARE_SIZE          # 0 = a, 7 = h
    rank_from_top = y // SQUARE_SIZE # 0 = rank 8, 7 = rank 1
    rank = 7 - rank_from_top         # 0 = rank 1, 7 = rank 8
    if 0 <= file < 8 and 0 <= rank < 8:
        return chess.square(file, rank)
    return None


def draw_board(screen, board, font, selected_sq=None, last_move=None, info_text=""):
    screen.fill(BG_COLOR)

    # Draw squares
    for rank in range(8):
        for file in range(8):
            sq = chess.square(file, rank)
            rank_from_top = 7 - rank
            x = file * SQUARE_SIZE
            y = rank_from_top * SQUARE_SIZE

            if (file + rank) % 2 == 0:
                color = LIGHT_COLOR
            else:
                color = DARK_COLOR

            pygame.draw.rect(screen, color, (x, y, SQUARE_SIZE, SQUARE_SIZE))

    # Highlight last move
    if last_move is not None:
        for sq in [last_move.from_square, last_move.to_square]:
            file = chess.square_file(sq)
            rank = chess.square_rank(sq)
            rank_from_top = 7 - rank
            x = file * SQUARE_SIZE
            y = rank_from_top * SQUARE_SIZE
            s = pygame.Surface((SQUARE_SIZE, SQUARE_SIZE), pygame.SRCALPHA)
            s.fill((*LASTMOVE_COLOR, 100))
            screen.blit(s, (x, y))

    # Highlight selected square
    if selected_sq is not None:
        file = chess.square_file(selected_sq)
        rank = chess.square_rank(selected_sq)
        rank_from_top = 7 - rank
        x = file * SQUARE_SIZE
        y = rank_from_top * SQUARE_SIZE
        s = pygame.Surface((SQUARE_SIZE, SQUARE_SIZE), pygame.SRCALPHA)
        s.fill((*HIGHLIGHT_COLOR, 120))
        screen.blit(s, (x, y))

    # Draw pieces
    for sq in chess.SQUARES:
        piece = board.piece_at(sq)
        if piece is None:
            continue
        file = chess.square_file(sq)
        rank = chess.square_rank(sq)
        rank_from_top = 7 - rank
        x = file * SQUARE_SIZE
        y = rank_from_top * SQUARE_SIZE

        symbol_tuple = PIECE_UNICODE.get(piece.piece_type)
        if not symbol_tuple:
            continue
        symbol = symbol_tuple[0] if piece.color == chess.WHITE else symbol_tuple[1]

        text_color = (10, 10, 10) if (file + rank) % 2 == 0 else (245, 245, 245)
        text_surf = font.render(symbol, True, text_color)
        text_rect = text_surf.get_rect(center=(x + SQUARE_SIZE//2, y + SQUARE_SIZE//2))
        screen.blit(text_surf, text_rect)

    # Draw info text at bottom
    if info_text:
        info_font = pygame.font.SysFont("Arial", 20)
        surf = info_font.render(info_text, True, (230, 230, 230))
        screen.blit(surf, (10, BOARD_SIZE_PX + 5))


def ask_promotion(board, from_sq, to_sq):
    # Simplest: always queen; can be extended with a popup later
    return chess.QUEEN


# ====== Main GUI loop ======

def play_gui(engine_path=ENGINE_PATH):
    # Init pygame
    pygame.init()
    pygame.display.set_caption("AloEngine GUI")
    total_height = BOARD_SIZE_PX + 30
    screen = pygame.display.set_mode((BOARD_SIZE_PX, total_height))
    clock = pygame.time.Clock()

    piece_font = pygame.font.SysFont("DejaVu Sans", 48)

    # Init engine (verbose=True logs flow to terminal)
    engine = UCIEngine(engine_path, verbose=True)
    print(f"[INFO] Starting engine: {engine_path}", flush=True)
    engine.start()
    engine.new_game()

    board = chess.Board()
    moves_uci = []

    human_color = chess.WHITE  # you as White for now
    engine_color = chess.BLACK

    selected_sq = None
    last_move = None
    info_text = "You are White. Click to move. ESC to quit, R to restart."

    running = True
    engine_thinking = False
    engine_move_result = queue.Queue()

    def engine_thread_func(moves_list_snapshot):
        try:
            print(f"[FLOW] Sending position to engine: {moves_list_snapshot}", flush=True)
            engine.set_position(moves_list_snapshot)
            print("[FLOW] Engine: go", flush=True)
            best = engine.go()
            engine_move_result.put(best)
        except Exception as e:
            engine_move_result.put(f"ERROR:{e}")

    while running:
        # Handle engine move completion
        if engine_thinking and not engine_move_result.empty():
            engine_thinking = False
            best = engine_move_result.get()
            if best is None or best.startswith("ERROR"):
                info_text = f"Engine error: {best}"
                print(f"[ERROR] {best}", flush=True)
            else:
                print(f"[ENGINE MOVE] {best}", flush=True)
                move = chess.Move.from_uci(best)
                if move in board.legal_moves:
                    board.push(move)
                    moves_uci.append(best)
                    last_move = move
                    info_text = f"Engine played: {best}"
                else:
                    info_text = f"Engine played illegal move: {best}"
                    print(f"[ERROR] Illegal engine move in position: {best}", flush=True)

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                if event.key == pygame.K_r:
                    # restart game
                    print("[INFO] Restarting game", flush=True)
                    board.reset()
                    moves_uci.clear()
                    selected_sq = None
                    last_move = None
                    info_text = "Game reset. You are White."
                    engine.new_game()
                    engine_thinking = False
                    while not engine_move_result.empty():
                        engine_move_result.get()

            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                if board.is_game_over():
                    info_text = f"Game over: {board.result()} (press R to restart)"
                    continue
                if board.turn != human_color:
                    continue
                if engine_thinking:
                    continue

                clicked_sq = square_from_mouse(event.pos)
                if clicked_sq is None:
                    continue

                piece = board.piece_at(clicked_sq)
                if selected_sq is None:
                    # select
                    if piece is None or piece.color != human_color:
                        continue
                    selected_sq = clicked_sq
                else:
                    # try move
                    from_sq = selected_sq
                    to_sq = clicked_sq

                    move = chess.Move(from_sq, to_sq)

                    # Promotion
                    if (board.piece_at(from_sq) is not None and
                        board.piece_at(from_sq).piece_type == chess.PAWN and
                        (chess.square_rank(to_sq) == 0 or chess.square_rank(to_sq) == 7)):
                        promo_piece = ask_promotion(board, from_sq, to_sq)
                        move = chess.Move(from_sq, to_sq, promotion=promo_piece)

                    if move in board.legal_moves:
                        board.push(move)
                        moves_uci.append(move.uci())
                        last_move = move
                        selected_sq = None
                        info_text = f"You played: {move.uci()}"
                        print(f"[HUMAN MOVE] {move.uci()}", flush=True)
                        print(f"[FLOW] Moves so far: {moves_uci}", flush=True)

                        # Engine reply
                        if not board.is_game_over():
                            engine_thinking = True
                            info_text = "Engine thinking..."
                            t = threading.Thread(
                                target=engine_thread_func,
                                args=(moves_uci.copy(),),
                                daemon=True,
                            )
                            t.start()
                    else:
                        # either reselect or clear
                        if piece is not None and piece.color == human_color:
                            selected_sq = clicked_sq
                        else:
                            selected_sq = None

        # Draw
        draw_board(screen, board, piece_font, selected_sq, last_move, info_text)
        pygame.display.flip()
        clock.tick(60)

    engine.stop()
    pygame.quit()
    sys.exit(0)


if __name__ == "__main__":
    try:
        play_gui()
    except KeyboardInterrupt:
        print("\n[INFO] Interrupted by user.")
        sys.exit(0)
