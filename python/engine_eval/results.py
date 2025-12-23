import csv
import time
from pathlib import Path
from typing import Dict, Tuple, List

from config import RESULTS_CSV

def ensure_results_header(path: Path = RESULTS_CSV) -> None:
    if path.exists():
        return
    with path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "game_id", "timestamp", "engine_a", "engine_b",
            "result_a", "color_a", "movetime_ms", "pgn_path"
        ])

def append_result(engine_a: str,
                  engine_b: str,
                  result_a: float,
                  color_a: str,
                  movetime_ms: int,
                  pgn_path: str | None,
                  path: Path = RESULTS_CSV) -> None:
    ensure_results_header(path)
    ts = int(time.time())
    # Make a simple game id
    game_id = f"{ts}_{engine_a}_vs_{engine_b}"
    with path.open("a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            game_id, ts, engine_a, engine_b,
            result_a, color_a, movetime_ms, pgn_path or ""
        ])

def load_results(path: Path = RESULTS_CSV) -> List[dict]:
    if not path.exists():
        return []
    rows: List[dict] = []
    with path.open("r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            # convert minimal types
            row["result_a"] = float(row["result_a"])
            row["movetime_ms"] = int(row["movetime_ms"])
            rows.append(row)
    # Already in chronological order because we append; no need to sort.
    return rows

def compute_pair_counts(rows: List[dict]) -> Dict[Tuple[str, str], int]:
    pair_counts: Dict[Tuple[str, str], int] = {}
    for r in rows:
        a = r["engine_a"]
        b = r["engine_b"]
        key = tuple(sorted((a, b)))
        pair_counts[key] = pair_counts.get(key, 0) + 1
    return pair_counts
