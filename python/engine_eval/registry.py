import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional

from config import ENGINES_JSON, DEFAULT_UNKNOWN_ELO, RATINGS_JSON

@dataclass
class EngineInfo:
    name: str
    path: str
    options: dict
    base_elo: Optional[float]   # from engines.json if present
    is_anchor: bool             # True if base_elo is defined
    rating: float               # current estimate (updated from results)
    games: int                  # games played

def load_engines(config_path: Path = ENGINES_JSON) -> Dict[str, EngineInfo]:
    data = json.loads(config_path.read_text())
    engines: Dict[str, EngineInfo] = {}
    for name, cfg in data.items():
        base_elo = cfg.get("elo")
        is_anchor = base_elo is not None
        engines[name] = EngineInfo(
            name=name,
            path=cfg["path"],
            options=cfg.get("options", {}),
            base_elo=float(base_elo) if base_elo is not None else None,
            is_anchor=is_anchor,
            rating=float(base_elo) if base_elo is not None else DEFAULT_UNKNOWN_ELO,
            games=0,
        )
    return engines

def load_ratings(engines: Dict[str, EngineInfo],
                 ratings_path: Path = RATINGS_JSON) -> None:
    if not ratings_path.exists():
        return
    data = json.loads(ratings_path.read_text())
    for name, info in engines.items():
        if name in data:
            rinfo = data[name]
            info.rating = float(rinfo["rating"])
            info.games = int(rinfo["games"])

def save_ratings(engines: Dict[str, EngineInfo],
                 ratings_path: Path = RATINGS_JSON) -> None:
    out = {
        name: {"rating": e.rating, "games": e.games}
        for name, e in engines.items()
    }
    ratings_path.write_text(json.dumps(out, indent=2))
