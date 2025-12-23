import random
from typing import Dict, Tuple, List

from registry import EngineInfo
from results import load_results, compute_pair_counts
from config import MAX_PAIR_GAMES

MIX_PROBABILITY = 0.3  # chance to pick a non-best opponent to avoid lock-in
LOWER_ELO_PROBABILITY = 0.25  # chance to prefer a lower-rated opponent when available

def choose_pair(engines: Dict[str, EngineInfo]) -> Tuple[EngineInfo, EngineInfo]:
    rows = load_results()
    pair_counts = compute_pair_counts(rows)

    # Prefer unknown engines
    unknowns = [e for e in engines.values() if not e.is_anchor]
    pool = unknowns if unknowns else list(engines.values())

    # Engine with fewest games
    min_games = min(e.games for e in pool) if pool else 0
    candidates = [e for e in pool if e.games == min_games]
    eng_a = random.choice(candidates)

    # Opponent selection: closest Elo, limited pair count
    others = [e for e in engines.values() if e.name != eng_a.name]
    if not others:
        raise RuntimeError("Need at least 2 engines to schedule a game")

    def pair_key(a: str, b: str) -> Tuple[str, str]:
        return tuple(sorted((a, b)))

    def opponent_score(e: EngineInfo) -> Tuple[float, int]:
        return (
            abs(e.rating - eng_a.rating),
            pair_counts.get(pair_key(eng_a.name, e.name), 0),
        )

    # Sort by Elo distance, then by fewest pair games
    others_sorted = sorted(others, key=opponent_score)

    # Filter out opponents with too many games vs A, if possible
    filtered: List[EngineInfo] = [
        e for e in others_sorted
        if pair_counts.get(pair_key(eng_a.name, e.name), 0) < MAX_PAIR_GAMES
    ]
    if not filtered:
        filtered = others_sorted

    # If available, occasionally pick a lower-rated opponent to encourage variety.
    if random.random() < LOWER_ELO_PROBABILITY:
        lower = [e for e in filtered if e.rating < eng_a.rating]
        if lower:
            return eng_a, random.choice(lower)

    # Mix in a random non-best choice sometimes to avoid exclusive pairing.
    if len(filtered) > 1 and random.random() < MIX_PROBABILITY:
        return eng_a, random.choice(filtered[1:])

    return eng_a, filtered[0]
