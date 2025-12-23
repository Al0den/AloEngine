from typing import Dict, List

from registry import EngineInfo
from results import load_results
from config import DEFAULT_UNKNOWN_ELO, K_FACTOR

def expected_score(ra: float, rb: float) -> float:
    return 1.0 / (1.0 + 10.0 ** ((rb - ra) / 400.0))

def recompute_ratings(engines: Dict[str, EngineInfo],
                      rows: List[dict] | None = None) -> None:
    # Reset to base/default
    if rows is None:
        rows = load_results()
    for e in engines.values():
        e.games = 0
        if e.base_elo is not None:
            e.rating = e.base_elo
        else:
            e.rating = DEFAULT_UNKNOWN_ELO

    for r in rows:
        a_name = r["engine_a"]
        b_name = r["engine_b"]
        result_a = r["result_a"]

        if a_name not in engines or b_name not in engines:
            continue

        ea = engines[a_name]
        eb = engines[b_name]

        ra = ea.rating
        rb = eb.rating

        sa = result_a
        sb = 1.0 - sa

        anch_a = ea.is_anchor
        anch_b = eb.is_anchor

        if anch_a and anch_b:
            # ignore for rating update
            ea.games += 1
            eb.games += 1
            continue

        # expected scores
        qa = expected_score(ra, rb)
        qb = expected_score(rb, ra)

        if anch_a and not anch_b:
            # Update only B
            eb.rating = rb + K_FACTOR * (sb - qb)
            eb.games += 1
            ea.games += 1
        elif anch_b and not anch_a:
            # Update only A
            ea.rating = ra + K_FACTOR * (sa - qa)
            ea.games += 1
            eb.games += 1
        else:
            # both unknown: update both
            ea.rating = ra + K_FACTOR * (sa - qa)
            eb.rating = rb + K_FACTOR * (sb - qb)
            ea.games += 1
            eb.games += 1
