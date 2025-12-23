from pathlib import Path

ROOT = Path(__file__).resolve().parents[0]  # AloEngine/
PYTHON_DIR = ROOT

ENGINES_JSON = PYTHON_DIR / "engines.json"

RESULTS_CSV = "../../data/results.csv"
RATINGS_JSON = "../../data/ratings.json"
PGN_DIR = "../../data/pgn/"
BOOK_FILE = "../../data/Perfect2021.bin"   # put your .bin here
DEFAULT_UNKNOWN_ELO = 2000.0
K_FACTOR = 20.0
MAX_PAIR_GAMES = 20  # max games per pair before deprioritising
