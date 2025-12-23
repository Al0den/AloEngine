from registry import load_engines, load_ratings, save_ratings
from results import append_result, load_results
from ratings import recompute_ratings
from scheduler import choose_pair
from play import play_single_game
from config import RESULTS_CSV


def main(num_games: int = 50, movetime_ms: int = 500):
    engines = load_engines()
    load_ratings(engines)

    rows = load_results(RESULTS_CSV)
    recompute_ratings(engines, rows)

    for i in range(num_games):
        eng_a, eng_b = choose_pair(engines)
        game_label = (
            f"Game {i + 1}/{num_games}: {eng_a.name} ({eng_a.rating:.1f}) "
            f"vs {eng_b.name} ({eng_b.rating:.1f})"
        )

        result_a, pgn_path, color_a = play_single_game(
            eng_a,
            eng_b,
            movetime_ms,
            display_backend="pygame",
            game_label=game_label,
            engine_log_io=False,
        )

        append_result(
            eng_a.name,
            eng_b.name,
            result_a,
            color_a,
            movetime_ms,
            pgn_path,
        )

        # Update in-memory rows for incremental rating update
        rows.append({
            "engine_a": eng_a.name,
            "engine_b": eng_b.name,
            "result_a": result_a,
            "movetime_ms": movetime_ms,
            "color_a": color_a,
            "game_id": "live",
            "timestamp": 0,
            "pgn_path": pgn_path,
        })
        recompute_ratings(engines, rows)
        save_ratings(engines)

        if result_a == 0.5:
            result_str = "1/2-1/2"
        elif result_a == 1.0:
            result_str = "1-0" if color_a == "white" else "0-1"
        else:
            result_str = "0-1" if color_a == "white" else "1-0"

        print(
            f"Result: {eng_a.name} vs {eng_b.name} -> {result_str} | "
            f"Ratings: {eng_a.name} {eng_a.rating:.1f}, "
            f"{eng_b.name} {eng_b.rating:.1f}"
        )


if __name__ == "__main__":
    main(num_games=100, movetime_ms=500)
