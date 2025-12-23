"""
uci_engine.py
-------------
Minimal but reliable UCI wrapper with:

- process spawn
- async-safe stdin/stdout handling
- new game handling
- position construction from python-chess board
- go movetime
- bestmove parsing
- terminal logging (Engine >> / Engine <<)

Used by the engine-eval framework.
"""

import subprocess
import threading
import queue
import time
import sys
import chess


class UCIEngine:
    def __init__(
        self,
        path: str,
        options: dict | None = None,
        name: str = "Engine",
        log_io: bool = False,
    ):
        self.path = path
        self.options = options or {}
        self.name = name
        self.log_io = log_io
        self.process = None
        self.stdout_queue = queue.Queue()
        self.stdout_thread = None
        self.alive = False

    # ----------------------------------------
    # Start / stop
    # ----------------------------------------

    def start(self):
        if self.alive:
            return

        self.process = subprocess.Popen(
            [self.path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )

        self.alive = True

        # Thread to read stdout asynchronously
        self.stdout_thread = threading.Thread(target=self._read_stdout, daemon=True)
        self.stdout_thread.start()

        self.send("uci")
        self._wait_for("uciok")

        # Apply engine options
        for k, v in self.options.items():
            self.send(f"setoption name {k} value {v}")

        self.send("isready")
        self._wait_for("readyok")

    def stop(self):
        if not self.alive:
            return
        try:
            self.send("quit")
            time.sleep(0.1)
        except Exception:
            pass
        if self.process:
            self.process.kill()
        self.alive = False

    # ----------------------------------------
    # Internal helpers
    # ----------------------------------------

    def _read_stdout(self):
        """Continuously read lines from engine stdout."""
        while True:
            if self.process is None:
                return
            line = self.process.stdout.readline()
            if not line:
                return
            line = line.rstrip("\n")
            self.stdout_queue.put(line)
            if self.log_io:
                print(f"[{self.name} <<] {line}")

    def send(self, cmd: str):
        """Send a command to engine."""
        if not self.alive:
            raise RuntimeError("Engine not started.")
        if self.log_io:
            print(f"[{self.name} >>] {cmd}")
        self.process.stdin.write(cmd + "\n")
        self.process.stdin.flush()

    def _wait_for(self, token: str, timeout: float = 5.0):
        """Wait until a line containing token is received."""
        t0 = time.time()
        while time.time() - t0 < timeout:
            try:
                line = self.stdout_queue.get(timeout=0.05)
                if token in line:
                    return line
            except queue.Empty:
                pass
        raise TimeoutError(f"{self.name}: Waiting for '{token}' timed out.")

    # ----------------------------------------
    # New game / position
    # ----------------------------------------

    def new_game(self):
        self.send("ucinewgame")
        self.send("isready")
        self._wait_for("readyok")

    def set_position_startpos(self, moves: list[str]):
        """Set position from startpos with a UCI move list."""
        if moves:
            cmd = "position startpos moves " + " ".join(moves)
        else:
            cmd = "position startpos"
        self.send(cmd)

    def set_position_board(self, board: chess.Board):
        """Constructs UCI 'position' command from a python-chess board."""
        moves = [m.uci() for m in board.move_stack]
        self.set_position_startpos(moves)

    # ----------------------------------------
    # Move selection
    # ----------------------------------------

    def choose_move(self, board: chess.Board, movetime_ms: int = 500) -> str:
        """Send go movetime and parse bestmove."""
        # Sync board → engine
        self.set_position_board(board)

        # Send go command
        self.send(f"go movetime {movetime_ms}")

        # Parse bestmove
        while True:
            line = self.stdout_queue.get()
            if "bestmove" in line:
                parts = line.split()
                if len(parts) >= 2:
                    return parts[1]
                break

        raise RuntimeError(f"{self.name}: bestmove not found.")
